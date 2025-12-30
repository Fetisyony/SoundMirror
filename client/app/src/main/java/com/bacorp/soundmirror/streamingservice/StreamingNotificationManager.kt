package com.bacorp.soundmirror.streamingservice

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.content.Context
import android.content.Context.NOTIFICATION_SERVICE
import android.content.Intent
import androidx.core.app.NotificationCompat
import com.bacorp.soundmirror.MainActivity
import com.bacorp.soundmirror.R

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
