package com.bacorp.soundmirror.streamingservice

import android.content.Intent
import android.os.Binder
import android.os.IBinder
import androidx.lifecycle.LifecycleService
import androidx.lifecycle.lifecycleScope
import com.bacorp.soundmirror.BuildConfig
import com.bacorp.soundmirror.streamingservice.data.StreamReceiverCoordinator
import com.bacorp.soundmirror.streamingservice.data.model.PlaybackState
import dagger.hilt.android.AndroidEntryPoint
import jakarta.inject.Inject
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.launch

@AndroidEntryPoint
class StreamReceiverService : LifecycleService() {
    @Inject lateinit var coordinator: StreamReceiverCoordinator
    private lateinit var notificationManager: StreamingNotificationManager

    inner class ServiceBinder : Binder() {
        val playbackState: StateFlow<PlaybackState> get() = coordinator.state

        fun stopStreaming() {
            this@StreamReceiverService.shutdownService()
        }
    }
    private val binder = ServiceBinder()

    override fun onBind(intent: Intent): IBinder? {
        super.onBind(intent)
        return binder
    }

    override fun onCreate() {
        super.onCreate()

        notificationManager = StreamingNotificationManager(this)

        notificationManager.createNotificationChannel()

        lifecycleScope.launch {
            coordinator.state.collect { state ->
                when (state) {
                    is PlaybackState.Playback -> {
                        val notification = notificationManager.buildNotification(state.ipAddress)
                        startForeground(NOTIFICATION_ID, notification)
                    }
                    is PlaybackState.Error -> {
                        stopForeground(STOP_FOREGROUND_REMOVE)
                    }
                    else -> {}
                }
            }
        }
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        super.onStartCommand(intent, flags, startId)
        val ipAddress = intent?.getStringExtra(EXTRA_IP) ?: return START_NOT_STICKY
        coordinator.startStreaming(ipAddress)
        return START_NOT_STICKY
    }

    private fun shutdownService() {
        stopForeground(STOP_FOREGROUND_REMOVE)

        coordinator.release()

        stopSelf()
    }

    companion object {
        private const val PKG = BuildConfig.APPLICATION_ID
        private val SERVICE_NAME = StreamReceiverService::class.java.simpleName

        val EXTRA_IP = "${PKG}.${SERVICE_NAME}.EXTRA_IP"

        private const val NOTIFICATION_ID = 1
    }
}
