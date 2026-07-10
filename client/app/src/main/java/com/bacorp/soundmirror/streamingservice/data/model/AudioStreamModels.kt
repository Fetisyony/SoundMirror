package com.bacorp.soundmirror.streamingservice.data.model

import kotlin.time.TimeSource

data class AudioFormatInfo(
    val sampleRate: Int,
    val channelConfig: Int,
    val audioFormat: Int,
    val bytesPerSample: Int
)

data class AudioChunk(
    val data: FloatArray,
    val sequenceId: Long,  // id received from the server, it is just a number that increments when server sends a new chunk of 10ms
    val emittingLatency: Long,
    val mark: TimeSource.Monotonic.ValueTimeMark  // TimeSource.Monotonic.markNow() when arrived
) {
    override fun equals(other: Any?): Boolean {
        if (this === other) return true
        if (javaClass != other?.javaClass) return false

        other as AudioChunk

        if (sequenceId != other.sequenceId) return false
        if (emittingLatency != other.emittingLatency) return false
        if (!data.contentEquals(other.data)) return false

        return true
    }

    override fun hashCode(): Int {
        var result = sequenceId.hashCode()
        result = 31 * result + emittingLatency.hashCode()
        result = 31 * result + data.contentHashCode()
        return result
    }
}
