package com.bacorp.soundmirror.presentation.viewmodel

import android.content.ComponentName
import android.content.Context
import android.content.Intent
import android.content.ServiceConnection
import android.os.IBinder
import androidx.core.content.ContextCompat
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.bacorp.soundmirror.R
import com.bacorp.soundmirror.domain.PreferencesRepository
import com.bacorp.soundmirror.presentation.state.UiState
import com.bacorp.soundmirror.streamingservice.StreamReceiverService
import com.bacorp.soundmirror.streamingservice.model.PlaybackState
import com.bacorp.soundmirror.utils.NetworkHelper
import dagger.hilt.android.lifecycle.HiltViewModel
import dagger.hilt.android.qualifiers.ApplicationContext
import jakarta.inject.Inject
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch

@HiltViewModel
class StreamingViewModel @Inject constructor(
    @ApplicationContext private val appContext: Context,
    private val settingsRepo: PreferencesRepository
) : ViewModel() {
    private val _uiState = MutableStateFlow(UiState())
    val uiState: StateFlow<UiState> = _uiState.asStateFlow()

    private var streamReceiverServiceBinder: StreamReceiverService.ServiceBinder? = null
    private var isBound = false

    private val connection = object : ServiceConnection {
        override fun onServiceConnected(className: ComponentName, service: IBinder) {
            val binder = service as StreamReceiverService.ServiceBinder
            streamReceiverServiceBinder = binder
            isBound = true

            viewModelScope.launch {
                binder.playbackState.collect { streamingState ->
                    updateStreamingStatus(streamingState)
                }
            }
        }

        override fun onServiceDisconnected(arg0: ComponentName) {
            isBound = false
            streamReceiverServiceBinder = null
            _uiState.update { it.copy(isStreaming = false, errorMessageCode = R.string.streaming_stopped) }
        }
    }

    init {
        viewModelScope.launch {
            settingsRepo.observeIpAddress()
                .collect { savedIp ->
                    if (savedIp.isEmpty()) {
                        val common = NetworkHelper.getCommonNetworkPart()
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
        if (!NetworkHelper.isIpFormatValid(currentIp)) {
            _uiState.update { it.copy(errorMessageCode = R.string.invalid_ip_address_format_error) }
            return
        }

        viewModelScope.launch {
            settingsRepo.saveIpAddress(currentIp)
        }

        startForegroundService(currentIp)
    }

    private fun startForegroundService(ip: String) {
        val context = appContext
        val intent = Intent(context, StreamReceiverService::class.java).apply {
            putExtra(StreamReceiverService.EXTRA_IP, ip)
        }
        ContextCompat.startForegroundService(context, intent)
        context.bindService(intent, connection, Context.BIND_AUTO_CREATE)
    }

    private fun stopStreaming() {
        if (isBound) {
            streamReceiverServiceBinder?.stopStreaming()
            appContext.unbindService(connection)
            isBound = false
            streamReceiverServiceBinder = null
        }
    }

    fun clearError() {
        _uiState.update { it.copy(errorMessageCode = null) }
    }

    override fun onCleared() {
        stopStreaming()
        super.onCleared()
    }

    fun updateStreamingStatus(playbackState: PlaybackState) {
        when (playbackState) {
            PlaybackState.Connecting -> _uiState.update { it.copy(isStreaming = false) }
            is PlaybackState.Error -> _uiState.update { it.copy(errorMessageCode = playbackState.messageResId) }
            PlaybackState.Idle -> _uiState.update { it.copy(isStreaming = false) }
            is PlaybackState.Playback -> _uiState.update { it.copy(isStreaming = true) }
        }
    }
}
