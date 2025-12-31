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
import java.util.PriorityQueue
import kotlin.time.TimeSource

class PlaybackManager @Inject constructor() : AudioConsumer {
    private var audioTrack: AudioTrack? = null

    private val jitterBuffer = PriorityQueue<AudioChunk> { a, b ->
        a.departmentTimestamp.compareTo(b.departmentTimestamp)
    }
    private var lastPlayedId: Long = 0

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

        val sc = TimeSource.Monotonic
        while (currentCoroutineContext().isActive) {
            while (true) {
                val res = audioQueue.tryReceive()
                if (res.isSuccess) {
                    val chunk = res.getOrThrow()
                    if (chunk.departmentTimestamp > lastPlayedId)
                        jitterBuffer.add(chunk)
                } else {
                    break
                }
            }

            if (jitterBuffer.isEmpty()) {
                val chunk = audioQueue.receive()
                if (chunk.departmentTimestamp > lastPlayedId) {
                    jitterBuffer.add(chunk)
                }
            }

            val next = jitterBuffer.peek()

            if (next != null) {
                val latency = (sc.markNow() - next.mark).inWholeMilliseconds
                if (latency > 180) {
                    Log.w("PB_MANAGER", "Dropping old packet. Latency: $latency ms")
                    jitterBuffer.poll()
                    lastPlayedId = next.departmentTimestamp
                    continue
                }

                jitterBuffer.poll()

                lastPlayedId = next.departmentTimestamp
                println((sc.markNow() - next.mark).inWholeMilliseconds)
                playChunk(next.data)
            }
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
    }
}
