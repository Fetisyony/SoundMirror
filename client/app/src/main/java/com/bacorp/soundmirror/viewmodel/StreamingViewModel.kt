package com.bacorp.soundmirror.viewmodel

import android.app.Application
import android.content.Intent
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.bacorp.soundmirror.R
import com.bacorp.soundmirror.data.NetworkHelper
import com.bacorp.soundmirror.data.SettingsRepository
import com.bacorp.soundmirror.service.StreamingService
import com.bacorp.soundmirror.state.UiState
import kotlinx.coroutines.flow.*
import kotlinx.coroutines.launch

class StreamingViewModel(
    private val app: Application
) : AndroidViewModel(app) {

    private val settingsRepo = SettingsRepository(app.applicationContext)
    private val networkHelper = NetworkHelper()

    private val _uiState = MutableStateFlow(UiState())
    val uiState: StateFlow<UiState> = _uiState.asStateFlow()

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

    fun onConnectClicked() {
        val currentIp = uiState.value.ipAddress.trim()
        if (!settingsRepo.isValidIpAddress(currentIp)) {
            _uiState.update { it.copy(errorMessageCode = R.string.invalid_ip_address_format_error) }
            return
        }

        viewModelScope.launch {
            settingsRepo.saveIpAddress(currentIp)
        }

        if (_uiState.value.isStreaming) {
            stopService()
        } else {
            startService(currentIp)
        }
        _uiState.update { it.copy(isStreaming = !_uiState.value.isStreaming) }
    }

    private fun startService(ip: String) {
        val context = app.applicationContext
        val startIntent = Intent(context, StreamingService::class.java).apply {
            action = StreamingService.ACTION_START
            putExtra(StreamingService.EXTRA_IP, ip)
        }
        context.startForegroundService(startIntent)
    }

    private fun stopService() {
        val context = app.applicationContext
        val stopIntent = Intent(context, StreamingService::class.java).apply {
            action = StreamingService.ACTION_STOP
        }
        context.startService(stopIntent)
    }

    fun clearError() {
        _uiState.update { it.copy(errorMessageCode = null) }
    }
}
