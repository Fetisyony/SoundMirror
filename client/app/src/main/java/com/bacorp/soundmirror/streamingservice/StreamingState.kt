package com.bacorp.soundmirror.streamingservice


sealed class StreamingState {
    object Idle : StreamingState()
    object Connecting : StreamingState()
    data class Streaming(val ipAddress: String) : StreamingState()
    data class Error(val messageResId: Int) : StreamingState()
}
