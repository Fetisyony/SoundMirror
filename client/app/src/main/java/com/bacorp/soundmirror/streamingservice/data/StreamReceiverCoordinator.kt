package com.bacorp.soundmirror.streamingservice.data

import android.util.Log
import com.bacorp.soundmirror.R
import com.bacorp.soundmirror.streamingservice.data.model.AudioChunk
import com.bacorp.soundmirror.streamingservice.domain.AudioConsumer
import com.bacorp.soundmirror.streamingservice.domain.AudioStreamClient
import com.bacorp.soundmirror.streamingservice.data.model.PlaybackConstants
import com.bacorp.soundmirror.streamingservice.data.model.PlaybackState
import com.bacorp.soundmirror.timeservice.TimeService
import jakarta.inject.Inject
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.cancel
import kotlinx.coroutines.cancelAndJoin
import kotlinx.coroutines.channels.Channel
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.io.DataOutputStream
import java.io.IOException
import java.net.InetSocketAddress
import java.net.Socket
import java.net.SocketTimeoutException
import java.net.UnknownHostException
import java.nio.ByteBuffer
import java.nio.ByteOrder
import kotlin.coroutines.cancellation.CancellationException

class StreamReceiverCoordinator @Inject constructor(
    private val streamReceiver: AudioStreamClient,
    private val consumer: AudioConsumer,
    private val timeService: TimeService
) {
    private val _state = MutableStateFlow<PlaybackState>(PlaybackState.Idle)
    val state: StateFlow<PlaybackState> = _state.asStateFlow()

    private val scope = CoroutineScope(Dispatchers.IO + SupervisorJob())
    private var streamingJob: Job? = null

    private val audioQueue = Channel<AudioChunk>(Channel.UNLIMITED)

    fun startStreaming(ip: String) {
        if (state.value is PlaybackState.Playback || state.value is PlaybackState.Connecting) {
            return
        }

        streamingJob?.cancel()
        streamingJob = scope.launch {
            try {
                _state.value = PlaybackState.Connecting

                val masterSocket = Socket().apply {
                    connect(InetSocketAddress(ip, PlaybackConstants.CONTROL_PORT))
                }
                val masterOutputStream = DataOutputStream(masterSocket!!.getOutputStream())

                val buffer = ByteBuffer.allocate(4).order(ByteOrder.LITTLE_ENDIAN)
                buffer.putInt(50)
                masterOutputStream.write(buffer.array())
                masterOutputStream.flush()

                timeService.initialize(ip, PlaybackConstants.PORT_SYNC)
                timeService.sync()

                streamReceiver.initialize(ip,
                    PlaybackConstants.PORT_STREAMING,
                    PlaybackConstants.TIMEOUT_MILLIS
                )
                val formatInfo = streamReceiver.getFormatInfo()
                println("${formatInfo.sampleRate}")

                streamReceiver.sendReceiverPort()

                if (!consumer.initialize(formatInfo)) {
                    throw IllegalStateException("Player failed to initialize")
                }
                _state.value = PlaybackState.Playback(ip)

                launch(Dispatchers.IO) {
                    streamReceiver.startReceiving(audioQueue)
                }

                launch(Dispatchers.Default) {
                    consumer.processQueue(audioQueue)
                }
            } catch (e: Exception) {
                if (e !is CancellationException) {
                    handleStreamingError(e)
                }
            }
        }
    }

    private fun handleStreamingError(e: Exception) {
        Log.d("StreamCoordinator", "Streaming error: ${e.message}", e)
        val errorResId = when(e) {
            is UnknownHostException -> R.string.error_connection_failed
            is SocketTimeoutException -> R.string.error_connection_timeout
            is IOException -> R.string.error_network_io
            is IllegalArgumentException -> R.string.error_unsupported_audio_format
            is IllegalStateException -> R.string.error_audio_setup_failed
            else -> R.string.error_service_unknown
        }
        _state.value = PlaybackState.Error(errorResId)
    }

    private fun stopInternal() {
        while (audioQueue.tryReceive().isSuccess) {}
        Log.d("RECEIVER", "Stopped")
    }

    fun release() {
        _state.value = PlaybackState.Idle
        scope.launch {
            withContext(Dispatchers.IO) {
                streamReceiver.disconnect()
            }

            streamingJob?.cancelAndJoin()
            consumer.release()
            stopInternal()
            scope.cancel()
        }
    }
}