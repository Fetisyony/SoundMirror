package com.bacorp.soundmirror.streamingservice.domain

import com.bacorp.soundmirror.streamingservice.data.model.AudioChunk
import com.bacorp.soundmirror.streamingservice.data.model.AudioFormatInfo
import kotlinx.coroutines.channels.Channel

interface AudioConsumer {
    fun initialize(formatInfo: AudioFormatInfo): Boolean

    fun playChunk(chunk: FloatArray)

    suspend fun processQueue(audioQueue: Channel<AudioChunk>)

    fun release()
}
