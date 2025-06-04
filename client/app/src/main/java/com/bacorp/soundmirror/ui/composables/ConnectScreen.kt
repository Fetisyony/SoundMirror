package com.bacorp.soundmirror.ui.composables

import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.animation.core.tween
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonColors
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalFocusManager
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.bacorp.soundmirror.R
import com.bacorp.soundmirror.state.UiState

@Composable
fun ConnectScreen(
    uiState: UiState,
    onIpChanged: (String) -> Unit,
    onConnectClicked: () -> Unit,
    onErrorDismissed: () -> Unit
) {
    val focusManager = LocalFocusManager.current

    val waveAmplitude by animateFloatAsState(
        targetValue = if (uiState.isStreaming) 60f else 10f,
        animationSpec = tween(durationMillis = 1000),
    )

    Scaffold(
        topBar = { ConnectionScreenTopBar() })
    { paddingValues ->
        Box(
            modifier = Modifier
                .fillMaxSize()
                .padding(paddingValues)
                .background(MaterialTheme.colorScheme.background)
        ) {
            Column(
                modifier = Modifier
                    .padding(vertical = 10.dp)
                    .align(Alignment.Center)
            ) {
                Spacer(modifier = Modifier.weight(1f))
                IpAddressInputField(
                    uiState.ipAddress, uiState.errorMessageCode, onIpChanged,
                    Modifier.padding(horizontal = 34.dp)
                )
                Spacer(modifier = Modifier.height(36.dp))
                Button(
                    colors =
                        ButtonColors(
                            MaterialTheme.colorScheme.primary,
                            MaterialTheme.colorScheme.onPrimary,
                            Color.Gray,
                            Color.White,
                        ),
                    modifier =
                        Modifier
                            .fillMaxWidth()
                            .padding(horizontal = 34.dp)
                            .height(52.dp),
                    shape = RoundedCornerShape(14.dp),
                    onClick = {
                        focusManager.clearFocus()
                        onConnectClicked()
                    },
                ) {
                    val text = if (uiState.isStreaming)
                        stringResource(R.string.disconnect_action)
                    else
                        stringResource(R.string.connect_action)
                    Text(
                        text = text,
                        fontSize = 18.sp
                    )
                }
                Spacer(modifier = Modifier.weight(1f))

                WaveVisualizer(
                    modifier = Modifier
                        .fillMaxWidth()
                        .height(100.dp),
                    amplitude = waveAmplitude
                )
            }

            if (uiState.errorMessageCode != null) {
                AlertDialog(
                    onDismissRequest = { onErrorDismissed() },
                    confirmButton = {
                        OutlinedButton(onClick = { onErrorDismissed() }) {
                            Text(stringResource(R.string.dialog_answer_ok))
                        }
                    },
                    title = { Text(stringResource(R.string.notification_title)) },
                    text = { Text(stringResource(uiState.errorMessageCode)) }
                )
            }
        }
    }
}
