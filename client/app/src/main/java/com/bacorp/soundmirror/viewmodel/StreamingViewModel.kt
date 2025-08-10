package com.bacorp.soundmirror.viewmodel

import android.app.Application
import android.content.ComponentName
import android.content.Context
import android.content.Intent
import android.content.ServiceConnection
import android.os.IBinder
import androidx.core.content.ContextCompat
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.bacorp.soundmirror.R
import com.bacorp.soundmirror.data.NetworkHelper
import com.bacorp.soundmirror.data.SettingsRepository
import com.bacorp.soundmirror.state.UiState
import com.bacorp.soundmirror.streamingservice.StreamingService
import com.bacorp.soundmirror.streamingservice.StreamingState
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch

class StreamingViewModel(
    private val app: Application
) : AndroidViewModel(app) {

    private val settingsRepo = SettingsRepository(app.applicationContext)
    private val networkHelper = NetworkHelper()

    private val _uiState = MutableStateFlow(UiState())
    val uiState: StateFlow<UiState> = _uiState.asStateFlow()

    private var streamingServiceBinder: StreamingService.ServiceBinder? = null
    private var isBound = false

    private val connection = object : ServiceConnection {
        override fun onServiceConnected(className: ComponentName, service: IBinder) {
            val binder = service as StreamingService.ServiceBinder
            streamingServiceBinder = binder
            isBound = true

            viewModelScope.launch {
                binder.streamingState.collect { streamingState ->
                    updateStreamingStatus(streamingState)
                }
            }
        }

        override fun onServiceDisconnected(arg0: ComponentName) {
            isBound = false
            streamingServiceBinder = null
            _uiState.update { it.copy(isStreaming = false, errorMessageCode = R.string.streaming_stopped) }
        }
    }

    init {
        viewModelScope.launch {
            settingsRepo.observeIpAddress()
                .collect { savedIp ->
                    if (savedIp.isEmpty()) {
                        val common = networkHelper.getCommonNetworkPart()
                        _uiState.update { it.copy(ipAddress = common ?: "") }
                    } else {
                        _uiState.update { it.copy(ipAddress = savedIp) }
                    }
                }
        }
    }

    fun onIpChanged(newIp: String) {
        _uiState.update { it.copy(ipAddress = newIp, errorMessageCode = null) }
    }

    fun onToggle() {
        if (!_uiState.value.isStreaming) {
            startStreaming()
        } else {
            stopStreaming()
        }
    }

    fun startStreaming() {
        val currentIp = uiState.value.ipAddress.trim()
        if (!settingsRepo.isValidIpAddress(currentIp)) {
            _uiState.update { it.copy(errorMessageCode = R.string.invalid_ip_address_format_error) }
            return
        }

        viewModelScope.launch {
            settingsRepo.saveIpAddress(currentIp)
        }

        startForegroundService(currentIp)
    }

    private fun startForegroundService(ip: String) {
        val context = app.applicationContext
        val intent = Intent(context, StreamingService::class.java).apply {
            putExtra(StreamingService.EXTRA_IP, ip)
        }
        ContextCompat.startForegroundService(context, intent)
        context.bindService(intent, connection, Context.BIND_AUTO_CREATE)
    }

    private fun stopStreaming() {
        if (isBound) {
            streamingServiceBinder?.stopStreaming()
            getApplication<Application>().applicationContext.unbindService(connection)
            isBound = false
            streamingServiceBinder = null
        }
    }

    fun clearError() {
        _uiState.update { it.copy(errorMessageCode = null) }
    }

    override fun onCleared() {
        stopStreaming()
        super.onCleared()
    }

    fun updateStreamingStatus(streamingState: StreamingState) {
        when (streamingState) {
            StreamingState.Connecting -> _uiState.update { it.copy(isStreaming = false) }
            is StreamingState.Error -> _uiState.update { it.copy(errorMessageCode = streamingState.messageResId) }
            StreamingState.Idle -> _uiState.update { it.copy(isStreaming = false) }
            is StreamingState.Streaming -> _uiState.update { it.copy(isStreaming = true) }
        }
    }
}
