package com.bacorp.soundmirror.presentation.state

enum class ConnectionState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED
}

data class UiState(
    val ipAddress: String = "",
    val connection: ConnectionState = ConnectionState.DISCONNECTED,
    val errorMessageCode: Int? = null
)
