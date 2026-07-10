package com.bacorp.soundmirror.streamingservice.data

import android.media.AudioFormat
import android.util.Log
import com.bacorp.soundmirror.streamingservice.data.model.AudioChunk
import com.bacorp.soundmirror.streamingservice.data.model.AudioFormatInfo
import com.bacorp.soundmirror.streamingservice.data.model.PlaybackConstants
import com.bacorp.soundmirror.streamingservice.data.model.PlaybackConstants.UDP_TIMEOUT_MILLIS
import com.bacorp.soundmirror.streamingservice.domain.AudioStreamClient
import com.bacorp.soundmirror.timeservice.TimeService
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.channels.Channel
import kotlinx.coroutines.currentCoroutineContext
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.flow
import kotlinx.coroutines.flow.flowOn
import kotlinx.coroutines.isActive
import java.io.DataInputStream
import java.io.DataOutputStream
import java.io.EOFException
import java.io.InputStream
import java.net.DatagramPacket
import java.net.DatagramSocket
import java.net.InetSocketAddress
import java.net.Socket
import java.net.SocketException
import java.net.SocketTimeoutException
import java.nio.ByteBuffer
import java.nio.ByteOrder
import javax.inject.Inject
import kotlin.coroutines.cancellation.CancellationException
import kotlin.time.TimeSource

class OpusWrapper {
    private var decoderPtr: Long = 0

    fun initDecoder(sampleRate: Int, channels: Int) {
        decoderPtr = nativeInit(sampleRate, channels)
    }

    fun decodeData(
        data: ByteArray,
        offset: Int,
        length: Int,
        outputBuffer: FloatArray
    ): Int {
        if (decoderPtr == 0L) return -1
        return nativeDecode(decoderPtr, data, offset, length, outputBuffer, 5760)
    }

    fun close() {
        if (decoderPtr != 0L) {
            nativeClose(decoderPtr)
            decoderPtr = 0
        }
    }

    private external fun nativeInit(sampleRate: Int, channels: Int): Long
    private external fun nativeDecode(ptr: Long, data: ByteArray, offset: Int, len: Int, out: FloatArray, frameSize: Int): Int
    private external fun nativeClose(ptr: Long)
}

class AudioStreamTcpClient @Inject constructor(
    private val timeService: TimeService
) : AudioStreamClient {
    private var managerSocket: Socket? = null
    private var udpSocket: DatagramSocket? = null
    private var channels: Int = 0

    val opusWrapper = OpusWrapper()

    private lateinit var managerInputStream: DataInputStream
    private lateinit var managerOutputStream: DataOutputStream

    private val streamBufferSize = 8200

    @Volatile
    private var isStreaming = false

    init {
        System.loadLibrary("native-lib")
    }

    override fun initialize(ip: String, port: Int, timeout: Int) {
        managerSocket = Socket().apply {
            connect(InetSocketAddress(ip, port), timeout)
        }
        managerInputStream = DataInputStream(managerSocket!!.getInputStream())
        managerOutputStream = DataOutputStream(managerSocket!!.getOutputStream())

        udpSocket = DatagramSocket(0)  // random port
        udpSocket?.soTimeout = UDP_TIMEOUT_MILLIS
    }

    override fun sendReceiverPort() {
        val udpPort = udpSocket!!.localPort
        sendUdpPort(udpPort)

        Log.d("AudioClient", "Initialized. Listening for UDP on port: $udpPort")
    }

    private fun sendUdpPort(port: Int) {
        try {
            val buffer = ByteBuffer.allocate(4).order(ByteOrder.LITTLE_ENDIAN)
            buffer.putInt(port)
            managerOutputStream.write(buffer.array())
            managerOutputStream.flush()
        } catch (e: Exception) {
            Log.e("AudioClient", "Failed to send UDP handshake: ${e.message}")
            throw e
        }
    }

    override fun getFormatInfo(): AudioFormatInfo {
        val headerBytes = ByteArray(PlaybackConstants.FORMAT_PACKET_SIZE)
        readFullyTcp(managerInputStream, headerBytes)
        return parseHeader(headerBytes)
    }

    override fun getStream(): Flow<AudioChunk> {
        return flow {
            isStreaming = true

            val packetArray = ByteArray(streamBufferSize)
            val packet = DatagramPacket(packetArray, packetArray.size)
            val wrapper = ByteBuffer.wrap(packetArray).order(ByteOrder.LITTLE_ENDIAN)

            while (isStreaming && udpSocket != null && !udpSocket!!.isClosed) {
                try {
                    packet.length = packetArray.size
                    udpSocket!!.receive(packet)
                    if (packet.length < PlaybackConstants.MESSAGE_HEADER_SIZE + PlaybackConstants.TIMESTAMP_SIZE) {
                        continue
                    }

                    wrapper.clear()
                    wrapper.limit(packet.length)

                    val chunkSize = wrapper.getInt()
                    if (chunkSize > packet.length - PlaybackConstants.TIMESTAMP_SIZE - 4) {
                        continue
                    }

                    val currentId = wrapper.getLong()

                    val samplesCount = chunkSize / Float.SIZE_BYTES
                    if (samplesCount == 0) continue

                    val floatBuffer = FloatArray(samplesCount)
                    wrapper.limit(wrapper.position() + chunkSize)
                    wrapper.asFloatBuffer().get(floatBuffer)

                    val latency = timeService.getServerTimeMillis() - currentId
                    emit(
                        AudioChunk(
                            data = floatBuffer,
                            sequenceId = currentId,
                            emittingLatency = latency,
                            mark = TimeSource.Monotonic.markNow()
                        )
                    )
                } catch (e: SocketTimeoutException) {
                    Log.w("AudioClient", "UDP Receive Timeout")
                } catch (e: Exception) {
                    if (isStreaming) {
                        Log.e("AudioClient", "Error receiving UDP: ${e.message}")
                    }
                    break
                }
            }
        }
            .flowOn(Dispatchers.IO)
    }

    override suspend fun startReceiving(audioQueue: Channel<AudioChunk>) {
        val packetArray = ByteArray(512)
        val packet = DatagramPacket(packetArray, packetArray.size)
        val wrapper = ByteBuffer.wrap(packetArray)

        val maxFrameSamplesPerChannel = 960
        val outputBuffer = FloatArray(maxFrameSamplesPerChannel * 2)

        val source = TimeSource.Monotonic
        while (currentCoroutineContext().isActive && udpSocket?.isClosed == false) {
            try {
                packet.length = packetArray.size
                udpSocket!!.receive(packet)

                wrapper.clear()
                wrapper.limit(packet.length)

                if (wrapper.remaining() < 12) continue

                val chunkSize = wrapper.getInt()
                val currentId = wrapper.getLong()

                if (chunkSize < 0 || chunkSize > (packet.length - 12)) {
                    Log.e("UDP", "Invalid payload size: $chunkSize (Available: ${wrapper.remaining()})")
                    continue
                }

                val samplesDecoded = opusWrapper.decodeData(
                    packetArray,
                    12,
                    chunkSize,
                    outputBuffer
                )

                if (samplesDecoded > 0) {
                    val res = audioQueue.trySend(
                        AudioChunk(
                            outputBuffer.copyOfRange(0, samplesDecoded * channels),
                            currentId,
                            0,
                            source.markNow()
                        )
                    )

                    if (res.isFailure) {
                        Log.d("UDP", "Not expecting this")
                    }
                }
            } catch (e: SocketException) {
                Log.d("UDP", "Socket closed, stopping receiver.")
                break
            } catch (e: Exception) {
                if (e is CancellationException) break
                Log.e("UDP", "Receive error: ${e.message}")
                throw e
            }
        }
    }

    override fun disconnect() {
        isStreaming = false
        try {
            val buffer = ByteBuffer.allocate(4).order(ByteOrder.LITTLE_ENDIAN)
            buffer.putInt(55)
            managerOutputStream.write(buffer.array())
            managerOutputStream.flush()
            Log.d("TCPCLIENT", "Sent 55 to the server")

            opusWrapper.close()
            managerSocket?.close()
            udpSocket?.close()
        } catch (e: Exception) {
            Log.w("AudioClient", "Error on disconnect: ${e.stackTraceToString()}")
        } finally {
            managerSocket = null
            udpSocket = null
        }
    }

    private fun readFullyTcp(input: InputStream, buf: ByteArray) {
        var pos = 0
        while (pos < buf.size) {
            val bytesRead = input.read(buf, pos, buf.size - pos)
            if (bytesRead < 0) throw EOFException("Stream ended prematurely")
            pos += bytesRead
        }
    }

    private fun parseHeader(headerBytes: ByteArray): AudioFormatInfo {
        val headerBuffer = ByteBuffer.wrap(headerBytes).order(ByteOrder.BIG_ENDIAN)
        val nChannels = headerBuffer.short.toInt() and 0xFFFF
        val bytesPerSample = headerBuffer.short.toInt() and 0xFFFF
        val sampleRate = headerBuffer.short.toInt() and 0xFFFF

        opusWrapper.initDecoder(sampleRate, nChannels)

        val channelConfig = when (nChannels) {
            1 -> AudioFormat.CHANNEL_OUT_MONO
            2 -> AudioFormat.CHANNEL_OUT_STEREO
            else -> throw IllegalArgumentException("Unsupported channel count: $nChannels")
        }
        val audioFormat = when (bytesPerSample) {
            1 -> AudioFormat.ENCODING_PCM_8BIT
            2 -> AudioFormat.ENCODING_PCM_16BIT
            4 -> AudioFormat.ENCODING_PCM_FLOAT
            else -> throw IllegalArgumentException("Unsupported bytesPerSample: $bytesPerSample")
        }

        channels = nChannels

        return AudioFormatInfo(sampleRate, channelConfig, audioFormat, bytesPerSample)
    }
}