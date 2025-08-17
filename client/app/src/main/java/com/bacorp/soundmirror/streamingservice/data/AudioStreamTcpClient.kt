package com.bacorp.soundmirror.streamingservice.data

import android.media.AudioFormat
import android.util.Log
import com.bacorp.soundmirror.streamingservice.domain.AudioStreamClient
import com.bacorp.soundmirror.streamingservice.data.model.AudioChunk
import com.bacorp.soundmirror.streamingservice.data.model.AudioFormatInfo
import com.bacorp.soundmirror.streamingservice.data.model.PlaybackConstants
import com.bacorp.soundmirror.timeservice.TimeService
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.buffer
import kotlinx.coroutines.flow.conflate
import kotlinx.coroutines.flow.flow
import kotlinx.coroutines.flow.flowOn
import kotlinx.coroutines.runBlocking
import java.io.DataInputStream
import java.io.EOFException
import java.io.InputStream
import java.net.InetSocketAddress
import java.net.Socket
import java.nio.ByteBuffer
import java.nio.ByteOrder
import javax.inject.Inject

class AudioStreamTcpClient @Inject constructor(
    private val timeService: TimeService
) : AudioStreamClient {
    private var socket: Socket? = null
    private lateinit var inputStream: InputStream

    override fun initialize(ip: String, port: Int, timeout: Int) {
        socket = Socket().apply {
            connect(InetSocketAddress(ip, port), timeout)
        }
        inputStream = DataInputStream(socket!!.getInputStream())
    }

    override fun getFormatInfo(): AudioFormatInfo {
        val headerBytes = ByteArray(PlaybackConstants.HEADER_SIZE)
        runBlocking { readFully(inputStream, headerBytes) }
        return parseHeader(headerBytes)
    }

    override fun getStream(): Flow<AudioChunk> {
        return flow {
            val messageHeaderBytes = ByteArray(PlaybackConstants.MESSAGE_HEADER_SIZE)
            val chunkHeaderBuffer =
                ByteBuffer.wrap(messageHeaderBytes).order(ByteOrder.LITTLE_ENDIAN)
            val timestampBytes = ByteArray(PlaybackConstants.TIMESTAMP_SIZE)
            val timestampBuffer = ByteBuffer.wrap(timestampBytes).order(ByteOrder.LITTLE_ENDIAN)

            while (true) {
                readFully(inputStream, messageHeaderBytes)
                val chunkSize = chunkHeaderBuffer.getInt(0)
                chunkHeaderBuffer.clear()

                readFully(inputStream, timestampBytes)
                val departmentTime = timestampBuffer.getLong(0)
                timestampBuffer.clear()

                val audioData = ByteArray(chunkSize)
                readFully(inputStream, audioData)

                val samplesCount = audioData.size / Float.SIZE_BYTES
                if (samplesCount == 0) continue
                val floatBuffer = FloatArray(samplesCount)
                ByteBuffer.wrap(audioData)
                    .order(ByteOrder.LITTLE_ENDIAN)
                    .asFloatBuffer()
                    .get(floatBuffer)

                val latency = timeService.getServerTimeMillis() - departmentTime
                Log.d("LATENCY_emitted", "$latency ms")
                emit(
                    AudioChunk(
                        data = floatBuffer,
                        departmentTimestamp = departmentTime,
                        emittingLatency = latency
                    )
                )
            }
        }
            .flowOn(Dispatchers.IO)
            .buffer(10)
            .conflate()
    }

    override fun disconnect() {
        try {
            socket?.close()
            inputStream.close()
        } catch (e: Exception) {
            Log.w("AudioStreamRepository", "Error on disconnect: ${e.message}")
        } finally {
            socket = null
        }
    }

    private suspend fun readFully(input: InputStream, buf: ByteArray) {
        var pos = 0
        while (pos < buf.size) {
            val bytesRead = input.read(buf, pos, buf.size - pos)
            if (bytesRead < 0) throw EOFException("Stream ended prematurely")
            if (bytesRead == 0) delay(1)
            pos += bytesRead
        }
    }

    private fun parseHeader(headerBytes: ByteArray): AudioFormatInfo {
        val headerBuffer = ByteBuffer.wrap(headerBytes).order(ByteOrder.BIG_ENDIAN)
        val nChannels = headerBuffer.short.toInt() and 0xFFFF
        val bytesPerSample = headerBuffer.short.toInt() and 0xFFFF
        val sampleRate = headerBuffer.short.toInt() and 0xFFFF

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

        return AudioFormatInfo(sampleRate, channelConfig, audioFormat, bytesPerSample)
    }
}