package com.bacorp.soundmirror.streamingservice.data

import ai.onnxruntime.OnnxTensor
import ai.onnxruntime.OrtEnvironment
import ai.onnxruntime.OrtSession
import android.media.AudioAttributes
import android.media.AudioFormat
import android.media.AudioTrack
import android.util.Log
import com.bacorp.soundmirror.streamingservice.data.model.AudioChunk
import com.bacorp.soundmirror.streamingservice.data.model.AudioFormatInfo
import com.bacorp.soundmirror.streamingservice.di.PlcModelBytes
import com.bacorp.soundmirror.streamingservice.domain.AudioConsumer
import jakarta.inject.Inject
import kotlinx.coroutines.channels.Channel
import kotlinx.coroutines.currentCoroutineContext
import kotlinx.coroutines.delay
import kotlinx.coroutines.isActive
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.nio.FloatBuffer
import kotlin.math.PI
import kotlin.math.max
import kotlin.math.sin

class PlaybackManager @Inject constructor(
    @PlcModelBytes private val modelBytes: ByteArray
) : AudioConsumer {
    private var audioTrack: AudioTrack? = null
    private val BUFFER_TARGET = 8
    private val CHUNK_DURATION_NS = 10_000_000L

    private var ortEnv: OrtEnvironment? = null
    private var ortSession: OrtSession? = null

    private val contextBuffer = FloatArray(6 * 160)
    private val contextDirectBuffer: FloatBuffer = ByteBuffer.allocateDirect(6 * 160 * 4)
        .order(ByteOrder.nativeOrder())
        .asFloatBuffer()
    private var olaBuffer = FloatArray(160)
    private val hannWindow = FloatArray(320)

    override fun initialize(formatInfo: AudioFormatInfo): Boolean {
        val minBufSize = AudioTrack.getMinBufferSize(
            formatInfo.sampleRate,
            formatInfo.channelConfig,
            formatInfo.audioFormat
        )
        if (minBufSize <= 0) {
            Log.d("PlaybackManager", "Invalid buffer size calculated: $minBufSize")
            return false
        }

        val bufferSizeInBytes = minBufSize * 2
        Log.d("PB_MANAGER", "Initialized with $bufferSizeInBytes")

        audioTrack = AudioTrack.Builder()
            .setAudioAttributes(
                AudioAttributes.Builder()
                    .setUsage(AudioAttributes.USAGE_MEDIA)
                    .setContentType(AudioAttributes.CONTENT_TYPE_MUSIC)
                    .build()
            )
            .setAudioFormat(
                AudioFormat.Builder()
                    .setEncoding(formatInfo.audioFormat)
                    .setSampleRate(formatInfo.sampleRate)
                    .setChannelMask(formatInfo.channelConfig)
                    .build()
            )
            .setBufferSizeInBytes(bufferSizeInBytes)
            .setTransferMode(AudioTrack.MODE_STREAM)
//            .setPerformanceMode(AudioTrack.PERFORMANCE_MODE_LOW_LATENCY)
            .build()

        if (audioTrack?.state != AudioTrack.STATE_INITIALIZED) {
            Log.d("PlaybackManager", "AudioTrack failed to initialize (state=${audioTrack?.state})")
            release()
            return false
        }

        audioTrack?.play()

        for (i in 0 until 320) {
            val sinVal = sin(PI * i / 320.0)
            hannWindow[i] = (sinVal * sinVal).toFloat()
        }

        try {
            ortEnv = OrtEnvironment.getEnvironment()
            val sessionOptions = OrtSession.SessionOptions().apply {
                addXnnpack(emptyMap())
            }
            ortSession = ortEnv?.createSession(modelBytes, sessionOptions)
        } catch (e: Exception) {
            Log.e("PlaybackManager", "Failed to load ONNX model", e)
            return false
        }

        return true
    }

    private fun pushToContext(frame: FloatArray) {
        System.arraycopy(contextBuffer, 160, contextBuffer, 0, contextBuffer.size - 160)
        System.arraycopy(frame, 0, contextBuffer, contextBuffer.size - 160, 160)
    }

    override fun playChunk(chunk: FloatArray) {
        val track = audioTrack ?: return
        val samplesCount = chunk.size
        if (samplesCount == 0) return

        var offset = 0
        while (offset < samplesCount) {
            val written = track.write(
                chunk,
                offset,
                samplesCount - offset,
                AudioTrack.WRITE_BLOCKING
            )
            if (written < 0) {
                Log.d("PlaybackManager", "AudioTrack.write returned error code: $written")
                break
            }
            offset += written
        }
    }

    private fun processStep(currentFrame: FloatArray?, nextFrame: FloatArray?, consecutiveLosses: Int): FloatArray {
        val isLossDetected = consecutiveLosses > 0
        val raw320 = FloatArray(320)

        if (!isLossDetected) {
            System.arraycopy(currentFrame!!, 0, raw320, 0, 160)
            System.arraycopy(nextFrame!!, 0, raw320, 160, 160)
            pushToContext(currentFrame)
        } else {
            Log.d("PlaybackManager", "Loss detected (Consecutive losses: $consecutiveLosses)")

            val shape = longArrayOf(1, 6, 160)
            contextDirectBuffer.clear()
            contextDirectBuffer.put(contextBuffer)
            contextDirectBuffer.flip()

            val tensor = OnnxTensor.createTensor(ortEnv, contextDirectBuffer, shape)

            try {
                val result = ortSession!!.run(mapOf("context" to tensor))
                val outTensor = result.get(0) as OnnxTensor
                outTensor.floatBuffer.get(raw320)
                result.close()
            } catch (e: Exception) {
                Log.e("PlaybackManager", "ONNX run failed", e)
            } finally {
                tensor.close()
            }

            val newCurrent = FloatArray(160)
            System.arraycopy(raw320, 0, newCurrent, 0, 160)
            pushToContext(newCurrent)
        }

        for (i in 0 until 320) {
            raw320[i] *= hannWindow[i]
        }

        if (isLossDetected) {
            // Volume decrease
            val fading = max(0f, 0.6f - (consecutiveLosses * 0.15f))
            for (i in 0 until 320) {
                raw320[i] *= fading
            }
        }

        val outputAudio = FloatArray(160)
        for (i in 0 until 160) {
            outputAudio[i] = raw320[i] + olaBuffer[i]
            olaBuffer[i] = raw320[i + 160]
        }

        return outputAudio
    }

    override suspend fun processQueue(audioQueue: Channel<AudioChunk>) {
        val buffer = mutableMapOf<Long, FloatArray>()
        var expectedId: Long = -1L
        var buffering = true
        var consecutiveLosses = 0

        var playoutStartTimeNs = 0L
        var startExpectedId = -1L

        contextBuffer.fill(0f)
        olaBuffer.fill(0f)

        while (currentCoroutineContext().isActive) {
            var res = audioQueue.tryReceive()
            while (res.isSuccess) {
                val chunk = res.getOrThrow()
                buffer[chunk.sequenceId] = chunk.data
                res = audioQueue.tryReceive()
            }

            if (buffering) {
                if (buffer.size >= BUFFER_TARGET) {
                    buffering = false
                    expectedId = buffer.keys.minOrNull() ?: -1L
                    startExpectedId = expectedId
                    playoutStartTimeNs = System.nanoTime()

                    audioTrack?.play()
                } else {
                    delay(2)
                    continue
                }
            }

            val threshold = expectedId - 10
            val iterator = buffer.iterator()
            while (iterator.hasNext()) {
                if (iterator.next().key < threshold) iterator.remove()
            }

            if (buffer.size > BUFFER_TARGET * 3) {
                val maxId = buffer.keys.maxOrNull() ?: expectedId
                expectedId = maxId - BUFFER_TARGET + 1
                startExpectedId = expectedId
                playoutStartTimeNs = System.nanoTime()
            }

            val nowNs = System.nanoTime()
            val chunkDeadlineNs = playoutStartTimeNs + (expectedId - startExpectedId) * CHUNK_DURATION_NS

            if (nowNs < chunkDeadlineNs) {
                val waitMs = (chunkDeadlineNs - nowNs) / 1_000_000L
                delay(max(1L, waitMs))
                continue
            }

            val currentFrame = buffer[expectedId]
            val nextFrame = buffer[expectedId + 1]

            val isLossDetected = currentFrame == null || nextFrame == null

            if (isLossDetected) {
                consecutiveLosses++

                if (consecutiveLosses >= 5) {
                    Log.w("PlaybackManager", "Pausing and rebuffering")
                    buffering = true
                    consecutiveLosses = 0

                    audioTrack?.pause()
                    audioTrack?.flush()

                    contextBuffer.fill(0f)
                    olaBuffer.fill(0f)
                    continue
                }
            } else {
                consecutiveLosses = 0
            }

            val outputAudio = processStep(currentFrame, nextFrame, consecutiveLosses)

            playChunk(outputAudio)

            buffer.remove(expectedId)
            ++expectedId
        }
    }

    override fun release() {
        audioTrack?.let {
            if (it.playState == AudioTrack.PLAYSTATE_PLAYING) {
                it.stop()
            }
            it.release()
        }
        audioTrack = null

        ortSession?.close()
        ortSession = null
        ortEnv?.close()
        ortEnv = null
    }
}
