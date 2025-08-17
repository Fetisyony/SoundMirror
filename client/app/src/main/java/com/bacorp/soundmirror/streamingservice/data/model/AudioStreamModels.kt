package com.bacorp.soundmirror.streamingservice.data.model

data class AudioFormatInfo(
    val sampleRate: Int,
    val channelConfig: Int,
    val audioFormat: Int,
    val bytesPerSample: Int
)

data class AudioChunk(
    val data: FloatArray,
    val departmentTimestamp: Long,
    val emittingLatency: Long
) {
    override fun equals(other: Any?): Boolean {
        if (this === other) return true
        if (javaClass != other?.javaClass) return false

        other as AudioChunk

        if (departmentTimestamp != other.departmentTimestamp) return false
        if (emittingLatency != other.emittingLatency) return false
        if (!data.contentEquals(other.data)) return false

        return true
    }

    override fun hashCode(): Int {
        var result = departmentTimestamp.hashCode()
        result = 31 * result + emittingLatency.hashCode()
        result = 31 * result + data.contentHashCode()
        return result
    }
}
