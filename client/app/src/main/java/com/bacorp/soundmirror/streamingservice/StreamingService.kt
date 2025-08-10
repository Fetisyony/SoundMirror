package com.bacorp.soundmirror.streamingservice

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.content.Context
import android.content.Context.NOTIFICATION_SERVICE
import android.content.Intent
import android.os.Binder
import android.os.IBinder
import androidx.core.app.NotificationCompat
import androidx.lifecycle.LifecycleService
import androidx.lifecycle.lifecycleScope
import com.bacorp.soundmirror.BuildConfig
import com.bacorp.soundmirror.MainActivity
import com.bacorp.soundmirror.R
import com.bacorp.soundmirror.streamingservice.timeservice.TimeService
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.launch

class StreamingService : LifecycleService() {
    private lateinit var coordinator: StreamCoordinator
    private lateinit var notificationManager: StreamingNotificationManager

    inner class ServiceBinder : Binder() {
        val streamingState: StateFlow<StreamingState> get() = coordinator.state

        fun stopStreaming() {
            this@StreamingService.shutdownService()
        }
    }
    private val binder = ServiceBinder()

    override fun onBind(intent: Intent): IBinder? {
        super.onBind(intent)
        return binder
    }

    override fun onCreate() {
        super.onCreate()

        val timeService = TimeService
        val repository = AudioStreamRepository()
        val player = PlaybackManager()
        coordinator = StreamCoordinator(repository, player, timeService)
        notificationManager = StreamingNotificationManager(this)

        notificationManager.createNotificationChannel()

        lifecycleScope.launch {
            coordinator.state.collect { state ->
                when (state) {
                    is StreamingState.Streaming -> {
                        val notification = notificationManager.buildNotification(state.ipAddress)
                        startForeground(NOTIFICATION_ID, notification)
                    }
                    is StreamingState.Error -> {
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
        private val SERVICE_NAME = StreamingService::class.java.simpleName

        val EXTRA_IP = "${PKG}.${SERVICE_NAME}.EXTRA_IP"

        private const val NOTIFICATION_ID = 1
    }
}

class StreamingNotificationManager(private val context: Context) {
    private val NOTIFICATION_CHANNEL_ID = "streaming_channel"
    private val NOTIFICATION_CHANNEL_NAME_CODE = R.string.audio_streaming_notification_channel_name

    fun createNotificationChannel() {
        val channel = NotificationChannel(
            NOTIFICATION_CHANNEL_ID,
            context.applicationContext.getString(NOTIFICATION_CHANNEL_NAME_CODE),
            NotificationManager.IMPORTANCE_LOW
        )
        channel.description = context.getString(R.string.streaming_channel_desctiption)
        val nm = context.getSystemService(NOTIFICATION_SERVICE) as NotificationManager
        nm.createNotificationChannel(channel)
    }

    fun buildNotification(ipAddress: String): Notification {
        val activityIntent = Intent(context, MainActivity::class.java).apply {
            flags = Intent.FLAG_ACTIVITY_CLEAR_TOP or Intent.FLAG_ACTIVITY_SINGLE_TOP
        }

        val pendingIntent = PendingIntent.getActivity(
            context,
            0,
            activityIntent,
            PendingIntent.FLAG_IMMUTABLE or PendingIntent.FLAG_UPDATE_CURRENT
        )

        return NotificationCompat.Builder(context, NOTIFICATION_CHANNEL_ID)
            .setContentTitle(context.getString(R.string.streaming_notification_title, ipAddress))
            .setContentText(context.getString(R.string.streaming_notification_action_description))
            .setSmallIcon(R.mipmap.ic_launcher_foreground)
            .setContentIntent(pendingIntent)
            .setOngoing(true)
            .build()
    }
}
