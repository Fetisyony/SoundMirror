package com.bacorp.soundmirror.streamingservice.domain

import com.bacorp.soundmirror.streamingservice.data.model.AudioFormatInfo

interface AudioConsumer {
    fun initialize(formatInfo: AudioFormatInfo): Boolean

    fun playChunk(chunk: FloatArray)

    fun release()
}
