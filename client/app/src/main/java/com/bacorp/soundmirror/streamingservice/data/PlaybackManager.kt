package com.bacorp.soundmirror.streamingservice.data

import android.media.AudioAttributes
import android.media.AudioFormat
import android.media.AudioTrack
import android.util.Log
import com.bacorp.soundmirror.streamingservice.domain.AudioConsumer
import com.bacorp.soundmirror.streamingservice.data.model.AudioFormatInfo
import jakarta.inject.Inject

class PlaybackManager @Inject constructor() : AudioConsumer {
    private var audioTrack: AudioTrack? = null

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

        val bufferSizeInBytes = minBufSize * 4

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