package com.bacorp.soundmirror.streamingservice

import android.util.Log
import com.bacorp.soundmirror.R
import com.bacorp.soundmirror.streamingservice.model.StreamingConstants.PORT_STREAMING
import com.bacorp.soundmirror.streamingservice.model.StreamingConstants.PORT_SYNC
import com.bacorp.soundmirror.streamingservice.model.StreamingConstants.TIMEOUT_MILLIS
import com.bacorp.soundmirror.streamingservice.model.StreamingState
import com.bacorp.soundmirror.streamingservice.model.StreamingState.Connecting
import com.bacorp.soundmirror.streamingservice.model.StreamingState.Streaming
import com.bacorp.soundmirror.timeservice.TimeService
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

class StreamCoordinator(
    private val repository: AudioStreamRepository,
    private val player: PlaybackManager,
    private val timeService: TimeService
) {
    private val _state = MutableStateFlow<StreamingState>(StreamingState.Idle)
    val state: StateFlow<StreamingState> = _state.asStateFlow()

    private val scope = CoroutineScope(Dispatchers.IO + SupervisorJob())
    private var streamingJob: Job? = null

    fun startStreaming(ip: String) {
        if (state.value is Streaming || state.value is Connecting) {
            return
        }

        streamingJob?.cancel()
        streamingJob = scope.launch {
            try {
                _state.value = Connecting

                timeService.initialize(ip, PORT_SYNC)
                timeService.sync()

                repository.initialize(ip, PORT_STREAMING, TIMEOUT_MILLIS)
                val formatInfo = repository.getFormatInfo()

                val streamFlow = repository.getStream()

                if (state.value is Connecting) {
                    if (!player.initialize(formatInfo)) {
                        throw IllegalStateException("Player failed to initialize")
                    }
                    _state.value = Streaming(ip)
                }

                streamFlow.collect { chunk ->
                    val latency = timeService.getServerTimeMillis() - chunk.departmentTimestamp
                    Log.d("LATENCY_collected", "$latency ms")

                    if (latency - chunk.emittingLatency < 100)
                        player.playChunk(chunk.data)
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
        _state.value = StreamingState.Error(errorResId)
    }

    private fun stopInternal() {
        repository.disconnect()
        player.release()
        _state.value = StreamingState.Idle
    }

    fun release() {
        scope.cancel()
        stopInternal()
    }
}
