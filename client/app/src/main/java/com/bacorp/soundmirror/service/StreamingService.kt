package com.bacorp.soundmirror.service

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.app.Service
import android.content.Intent
import android.media.AudioAttributes
import android.media.AudioFormat
import android.media.AudioTrack
import android.os.IBinder
import androidx.core.app.NotificationCompat
import com.bacorp.soundmirror.R
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.cancel
import kotlinx.coroutines.delay
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch
import java.io.DataInputStream
import java.io.EOFException
import java.io.InputStream
import java.net.Socket
import java.nio.ByteBuffer
import java.nio.ByteOrder
import com.bacorp.soundmirror.BuildConfig
import com.bacorp.soundmirror.MainActivity

class StreamingService : Service() {
    companion object {
        private const val PKG = BuildConfig.APPLICATION_ID

        private val SERVICE_NAME = StreamingService::class.java.simpleName

        val ACTION_START = "${PKG}.${SERVICE_NAME}.ACTION_START"
        val ACTION_STOP = "${PKG}.${SERVICE_NAME}.ACTION_STOP"
        val EXTRA_IP = "${PKG}.${SERVICE_NAME}.EXTRA_IP"

        private const val PORT = 20000
        private const val HEADER_SIZE = 6
        private const val NETWORK_BUFFER_SIZE = 4096

        private const val NOTIFICATION_CHANNEL_ID = "streaming_channel"
        private val NOTIFICATION_CHANNEL_NAME_CODE =
            R.string.audio_streaming_notification_channel_name
        private const val NOTIFICATION_ID = 1
    }

    private val serviceScope = CoroutineScope(SupervisorJob() + Dispatchers.IO)
    private var streamingJob: Job? = null

    override fun onBind(intent: Intent?): IBinder? = null

    override fun onCreate() {
        super.onCreate()
        createNotificationChannel()
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        intent?.action?.let { action ->
            when (action) {
                ACTION_START -> {
                    val ipAddress = intent.getStringExtra(EXTRA_IP) ?: return START_NOT_STICKY
                    startForeground(NOTIFICATION_ID, buildNotification(ipAddress))
                    beginStreaming(ipAddress)
                }

                ACTION_STOP -> {
                    stopStreaming()
                    stopForeground(STOP_FOREGROUND_REMOVE)
                    stopSelf()
                }
            }
        }
        return START_NOT_STICKY
    }

    override fun onDestroy() {
        stopStreaming()
        serviceScope.cancel()
        super.onDestroy()
    }

    private fun beginStreaming(ip: String) {
        streamingJob?.cancel()

        streamingJob = serviceScope.launch {
            var socket: Socket? = null
            var audioTrack: AudioTrack? = null

            try {
                socket = Socket(ip, PORT)
                val input: InputStream = DataInputStream(socket.getInputStream())

                val headerBytes = ByteArray(HEADER_SIZE)
                readFully(input, headerBytes, 0, HEADER_SIZE)

                val headerBuffer = ByteBuffer.wrap(headerBytes).order(ByteOrder.BIG_ENDIAN)
                val nChannels = headerBuffer.short.toInt() and 0xFFFF
                val bytesPerSample = headerBuffer.short.toInt() and 0xFFFF
                val sampleRate = headerBuffer.short.toInt() and 0xFFFF

                val channelConfig = when (nChannels) {
                    1 -> AudioFormat.CHANNEL_OUT_MONO
                    2 -> AudioFormat.CHANNEL_OUT_STEREO
                    else -> throw IllegalArgumentException("Unsupported channel count: $nChannels")
                }
                val audioFormat = when (bytesPerSample) {
                    1 -> AudioFormat.ENCODING_PCM_8BIT
                    2 -> AudioFormat.ENCODING_PCM_16BIT
                    4 -> AudioFormat.ENCODING_PCM_FLOAT
                    else -> throw IllegalArgumentException("Unsupported bytesPerSample: $bytesPerSample")
                }

                val minBuf = AudioTrack.getMinBufferSize(sampleRate, channelConfig, audioFormat)
                if (minBuf <= 0) throw IllegalStateException("Invalid buffer size: $minBuf")

                val bufferSizeInBytes = minBuf * 4

                audioTrack = AudioTrack.Builder()
                    .setAudioAttributes(
                        AudioAttributes.Builder()
                            .setUsage(AudioAttributes.USAGE_MEDIA)
                            .setContentType(AudioAttributes.CONTENT_TYPE_MUSIC)
                            .build()
                    )
                    .setAudioFormat(
                        AudioFormat.Builder()
                            .setEncoding(audioFormat)
                            .setSampleRate(sampleRate)
                            .setChannelMask(channelConfig)
                            .build()
                    )
                    .setBufferSizeInBytes(bufferSizeInBytes)
                    .setTransferMode(AudioTrack.MODE_STREAM)
                    .setPerformanceMode(AudioTrack.PERFORMANCE_MODE_LOW_LATENCY)
                    .build()

                if (audioTrack.state != AudioTrack.STATE_INITIALIZED) {
                    throw IllegalStateException("AudioTrack failed to initialize (state=${audioTrack.state})")
                }
                audioTrack.play()

                val netBuffer = ByteArray(NETWORK_BUFFER_SIZE)
                val floatCount = NETWORK_BUFFER_SIZE / bytesPerSample
                val floatBuffer = FloatArray(floatCount)
                val tmpWrapper = ByteBuffer.wrap(netBuffer).order(ByteOrder.LITTLE_ENDIAN)
                val floatView = tmpWrapper.asFloatBuffer()

                while (isActive) {
                    readFully(input, netBuffer, 0, NETWORK_BUFFER_SIZE)
                    tmpWrapper.rewind()
                    floatView.rewind()
                    floatView.get(floatBuffer)

                    var offset = 0
                    while (offset < floatCount) {
                        val written = audioTrack.write(
                            floatBuffer, offset, floatCount - offset,
                            AudioTrack.WRITE_BLOCKING
                        )
                        if (written < 0) {
                            throw IllegalStateException("AudioTrack.write returned error code $written")
                        }
                        offset += written
                    }
                }
            } catch (e: Exception) {
                e.printStackTrace()
            } finally {
                audioTrack?.let {
                    it.stop()
                    it.release()
                }
                socket?.close()
            }
        }
    }

    private suspend fun readFully(
        input: InputStream,
        buf: ByteArray,
        offset: Int,
        length: Int
    ) {
        var pos = 0
        while (pos < length) {
            val r = input.read(buf, offset + pos, length - pos)
            if (r < 0) throw EOFException("Stream ended prematurely")
            if (r == 0) delay(1)
            pos += r
        }
    }

    private fun stopStreaming() {
        streamingJob?.cancel()
        streamingJob = null
    }

    private fun createNotificationChannel() {
        val channel = NotificationChannel(
            NOTIFICATION_CHANNEL_ID,
            applicationContext.getString(NOTIFICATION_CHANNEL_NAME_CODE),
            NotificationManager.IMPORTANCE_LOW
        )
        channel.description = getString(R.string.streaming_channel_desctiption)
        val nm = getSystemService(NOTIFICATION_SERVICE) as NotificationManager
        nm.createNotificationChannel(channel)
    }

    private fun buildNotification(ipAddress: String): Notification {
        val activityIntent = Intent(this, MainActivity::class.java).apply {
            flags = Intent.FLAG_ACTIVITY_CLEAR_TOP or Intent.FLAG_ACTIVITY_SINGLE_TOP
        }

        val pendingIntent = PendingIntent.getActivity(
            this,
            0,
            activityIntent,
            PendingIntent.FLAG_IMMUTABLE or PendingIntent.FLAG_UPDATE_CURRENT
        )

        return NotificationCompat.Builder(this, NOTIFICATION_CHANNEL_ID)
            .setContentTitle(getString(R.string.streaming_notification_title, ipAddress))
            .setContentText(getString(R.string.streaming_notification_action_description))
            .setSmallIcon(R.mipmap.ic_launcher_foreground)
            .setContentIntent(pendingIntent)
            .setOngoing(true)
            .build()
    }
}
