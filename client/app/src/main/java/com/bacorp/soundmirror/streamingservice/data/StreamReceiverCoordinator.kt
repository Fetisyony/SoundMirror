package com.bacorp.soundmirror.streamingservice.data

import android.util.Log
import com.bacorp.soundmirror.R
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
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch
import java.io.IOException
import java.net.SocketTimeoutException
import java.net.UnknownHostException

class StreamReceiverCoordinator @Inject constructor(
    private val client: AudioStreamClient,
    private val consumer: AudioConsumer,
    private val timeService: TimeService
) {
    private val _state = MutableStateFlow<PlaybackState>(PlaybackState.Idle)
    val state: StateFlow<PlaybackState> = _state.asStateFlow()

    private val scope = CoroutineScope(Dispatchers.IO + SupervisorJob())
    private var streamingJob: Job? = null

    fun startStreaming(ip: String) {
        if (state.value is PlaybackState.Playback || state.value is PlaybackState.Connecting) {
            return
        }

        streamingJob?.cancel()
        streamingJob = scope.launch {
            try {
                _state.value = PlaybackState.Connecting

                timeService.initialize(ip, PlaybackConstants.PORT_SYNC)
                timeService.sync()

                client.initialize(ip,
                    PlaybackConstants.PORT_STREAMING,
                    PlaybackConstants.TIMEOUT_MILLIS
                )
                val formatInfo = client.getFormatInfo()
                println("${formatInfo.sampleRate}")

                val streamFlow = client.getStream()

                if (state.value is PlaybackState.Connecting) {
                    if (!consumer.initialize(formatInfo)) {
                        throw IllegalStateException("Player failed to initialize")
                    }
                    _state.value = PlaybackState.Playback(ip)
                }

                streamFlow.collect { chunk ->
                    val latency = timeService.getServerTimeMillis() - chunk.departmentTimestamp
                    Log.d("LATENCY_collected", "$latency ms ${Thread.currentThread().name}")

                    consumer.playChunk(chunk.data)
                }
            } catch (e: Exception) {
                handleStreamingError(e)
            } finally {
                stopInternal()
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
        client.disconnect()
        consumer.release()
        _state.value = PlaybackState.Idle
    }

    fun release() {
        scope.cancel()
        stopInternal()
    }
}