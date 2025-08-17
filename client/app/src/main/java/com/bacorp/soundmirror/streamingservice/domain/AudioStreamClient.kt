package com.bacorp.soundmirror.streamingservice.domain

import com.bacorp.soundmirror.streamingservice.data.model.AudioChunk
import com.bacorp.soundmirror.streamingservice.data.model.AudioFormatInfo
import kotlinx.coroutines.flow.Flow

interface AudioStreamClient {
    fun initialize(ip: String, port: Int, timeout: Int)

    fun getFormatInfo(): AudioFormatInfo

    fun getStream(): Flow<AudioChunk>

    fun disconnect()
}
