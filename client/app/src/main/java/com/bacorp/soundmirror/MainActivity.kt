package com.bacorp.soundmirror

import android.Manifest.permission.POST_NOTIFICATIONS
import android.annotation.SuppressLint
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.util.Log
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.result.contract.ActivityResultContracts
import androidx.activity.viewModels
import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.core.content.ContextCompat
import com.bacorp.soundmirror.streamingservice.ServiceConstants
import com.bacorp.soundmirror.ui.composables.ConfigureStatusBarColor
import com.bacorp.soundmirror.ui.composables.ConnectScreen
import com.bacorp.soundmirror.ui.theme.SoundMirrorTheme
import com.bacorp.soundmirror.viewmodel.StreamingViewModel
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import java.io.DataInputStream
import java.io.DataOutputStream
import java.net.InetSocketAddress
import java.net.Socket
import java.net.SocketTimeoutException

class MainActivity : ComponentActivity() {
    private val viewModel: StreamingViewModel by viewModels()
    private val notificationPermissionLauncher =
        registerForActivityResult(ActivityResultContracts.RequestPermission()) { granted ->
            if (!granted) {
                Toast.makeText(
                    this,
                    getString(R.string.why_notification_required_explanation),
                    Toast.LENGTH_LONG
                ).show()
            }
        }

    private val serviceReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context?, intent: Intent?) {
            if (intent?.action == ServiceConstants.ACTION_SERVICE_ERROR) {
                val errorMessageCode = intent.getIntExtra(
                    ServiceConstants.EXTRA_ERROR_MESSAGE_CODE,
                    R.string.unknown_background_error
                )
                viewModel.handleServiceError(errorMessageCode)
            } else if (intent?.action == ServiceConstants.ACTION_SERVICE_STATUS_UPDATE) {
                val isRunning =
                    intent.getBooleanExtra(ServiceConstants.EXTRA_SERVICE_RUNNING, false)
                viewModel.updateRunningStatus(isRunning)
            }
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            if (ContextCompat.checkSelfPermission(
                    this,
                    POST_NOTIFICATIONS
                ) != PackageManager.PERMISSION_GRANTED
            ) {
                notificationPermissionLauncher.launch(POST_NOTIFICATIONS)
            }
        }

        val filter = IntentFilter().apply {
            addAction(ServiceConstants.ACTION_SERVICE_ERROR)
            addAction(ServiceConstants.ACTION_SERVICE_STATUS_UPDATE)
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            registerReceiver(serviceReceiver, filter, RECEIVER_EXPORTED)
        } else {
            @SuppressLint("UnspecifiedRegisterReceiverFlag")
            registerReceiver(serviceReceiver, filter)
        }

        setContent {
            val uiState by viewModel.uiState.collectAsState()

            SoundMirrorTheme(
                dynamicColor = false,
            ) {
                ConfigureStatusBarColor(window, isSystemInDarkTheme())

                ConnectScreen(
                    uiState = uiState,
                    onIpChanged = { viewModel.onIpChanged(it) },
                    onConnectClicked = { viewModel.onConnectClicked() },
                    onErrorDismissed = { viewModel.clearError() }
                )
            }
        }
    }
}
