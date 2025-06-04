package com.bacorp.soundmirror

import android.Manifest.permission.POST_NOTIFICATIONS
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.result.contract.ActivityResultContracts
import androidx.activity.viewModels
import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.core.content.ContextCompat
import com.bacorp.soundmirror.ui.composables.ConfigureStatusBarColor
import com.bacorp.soundmirror.ui.composables.ConnectScreen
import com.bacorp.soundmirror.ui.theme.SoundMirrorTheme
import com.bacorp.soundmirror.viewmodel.StreamingViewModel

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
