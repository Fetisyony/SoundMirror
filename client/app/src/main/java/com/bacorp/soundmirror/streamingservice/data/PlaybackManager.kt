package com.bacorp.soundmirror.streamingservice.data

import android.media.AudioAttributes
import android.media.AudioFormat
import android.media.AudioTrack
import android.util.Log
import com.bacorp.soundmirror.streamingservice.data.model.AudioChunk
import com.bacorp.soundmirror.streamingservice.data.model.AudioFormatInfo
import com.bacorp.soundmirror.streamingservice.domain.AudioConsumer
import jakarta.inject.Inject
import kotlinx.coroutines.channels.Channel
import kotlinx.coroutines.currentCoroutineContext
import kotlinx.coroutines.isActive
import kotlinx.coroutines.yield
import java.util.PriorityQueue

class PlaybackManager @Inject constructor() : AudioConsumer {
    private var audioTrack: AudioTrack? = null

    private var lastChunk: FloatArray = FloatArray(0)
    private val jitterBuffer = PriorityQueue<AudioChunk> { a, b ->
        (a.departmentTimestamp - b.departmentTimestamp).toInt()
    }
    private var lastPlayedId: Long = 0
    private val BUFFER_TARGET = 10

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

        val bufferSizeInBytes = minBufSize * 6
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
            .setPerformanceMode(AudioTrack.PERFORMANCE_MODE_LOW_LATENCY)
            .build()

        if (audioTrack?.state != AudioTrack.STATE_INITIALIZED) {
            Log.d("PlaybackManager", "AudioTrack failed to initialize (state=${audioTrack?.state})")
            release()
            return false
        }

        audioTrack?.play()
        return true
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

    override suspend fun processQueue(audioQueue: Channel<AudioChunk>) {
        jitterBuffer.clear()
        lastPlayedId = 0

        var isInitialBuffering = true
        var isRebuffering = false
        val MAX_BUFFER_SIZE = BUFFER_TARGET * 10

        while (currentCoroutineContext().isActive) {
            val result = audioQueue.tryReceive()
            if (result.isSuccess) {
                jitterBuffer.add(result.getOrThrow())
            }
            var i = 0
            while (isRebuffering) {
                jitterBuffer.add(audioQueue.receive())
                if (jitterBuffer.size >= BUFFER_TARGET)
                    isRebuffering = false
                if (i * 2 > BUFFER_TARGET)
                    break
                ++i
            }

            if (isInitialBuffering) {
                if (jitterBuffer.size < BUFFER_TARGET) {
                    val chunk = audioQueue.receive()
                    jitterBuffer.add(chunk)
                    continue
                } else {
                    Log.d("PB_MANAGER", "Buffer refilled to ${BUFFER_TARGET}! Resuming playback.")
                    isInitialBuffering = false
                }
            }

            if (jitterBuffer.isNotEmpty()) {
                val next = jitterBuffer.peek()

                if (jitterBuffer.size > MAX_BUFFER_SIZE || next.departmentTimestamp + 2 <= lastPlayedId) {
                    Log.d("PB_MANAGER", "Dropping late/extra/duplicate packet: ${jitterBuffer.size} vs ${MAX_BUFFER_SIZE}")
                    jitterBuffer.poll()
                    continue
                }

                playChunk(next.data)

                lastPlayedId = next.departmentTimestamp
                jitterBuffer.poll()

                lastChunk = next.data
            } else {
                Log.d("PB_MANAGER", "Empty: switch to rebuffering")
                isRebuffering = true

                playChunk(lastChunk)
            }
            yield()
        }

        println("Cancelled")
    }

    override fun release() {
        audioTrack?.let {
            if (it.playState == AudioTrack.PLAYSTATE_PLAYING) {
                it.stop()
            }
            it.release()
        }
        audioTrack = null
    }
}
