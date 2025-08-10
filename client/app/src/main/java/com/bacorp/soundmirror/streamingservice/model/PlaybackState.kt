package com.bacorp.soundmirror.streamingservice.model

sealed class PlaybackState {
    object Idle : PlaybackState()
    object Connecting : PlaybackState()
    data class Playback(val ipAddress: String) : PlaybackState()
    data class Error(val messageResId: Int) : PlaybackState()
}
