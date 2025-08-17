package com.bacorp.soundmirror.streamingservice.data.model

sealed class PlaybackState {
    object Idle : PlaybackState()
    object Connecting : PlaybackState()
    data class Playback(val ipAddress: String) : PlaybackState()
    data class Error(val messageResId: Int) : PlaybackState()
}
