package com.bacorp.soundmirror.streamingservice.domain

import com.bacorp.soundmirror.streamingservice.data.model.AudioChunk
import com.bacorp.soundmirror.streamingservice.data.model.AudioFormatInfo
import kotlinx.coroutines.channels.Channel
import kotlinx.coroutines.flow.Flow

interface AudioStreamClient {
    fun initialize(ip: String, port: Int, timeout: Int)

    fun sendReceiverPort();

    fun getFormatInfo(): AudioFormatInfo

    fun getStream(): Flow<AudioChunk>

    suspend fun startReceiving(audioQueue: Channel<AudioChunk>)

    fun disconnect()
}
