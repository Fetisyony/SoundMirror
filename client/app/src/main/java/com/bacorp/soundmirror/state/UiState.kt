package com.bacorp.soundmirror.state

data class UiState(
    val ipAddress: String = "",
    val isStreaming: Boolean = false,
    val errorMessageCode: Int? = null
)
