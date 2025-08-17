package com.bacorp.soundmirror.streamingservice.di

import com.bacorp.soundmirror.streamingservice.data.AudioStreamTcpClient
import com.bacorp.soundmirror.streamingservice.data.PlaybackManager
import com.bacorp.soundmirror.streamingservice.data.StreamReceiverCoordinator
import com.bacorp.soundmirror.streamingservice.domain.AudioConsumer
import com.bacorp.soundmirror.streamingservice.domain.AudioStreamClient
import com.bacorp.soundmirror.timeservice.TimeService
import dagger.Binds
import dagger.Module
import dagger.Provides
import dagger.hilt.InstallIn
import dagger.hilt.android.components.ServiceComponent
import dagger.hilt.android.scopes.ServiceScoped

@Module
@InstallIn(ServiceComponent::class)
abstract class ServiceModule {
    @Binds
    abstract fun bindsAudioConsumer(impl: PlaybackManager): AudioConsumer

    companion object {
        @Provides @ServiceScoped
        fun providesTimeSyncService(): TimeService = TimeService()

        @Provides
        fun bindsAudioStreamClient(
            timeService: TimeService
        ): AudioStreamClient {
            return AudioStreamTcpClient(timeService)
        }

        @Provides
        fun providesCoordinator(
            client: AudioStreamClient,
            consumer: AudioConsumer,
            timeService: TimeService
        ): StreamReceiverCoordinator = StreamReceiverCoordinator(client, consumer, timeService)
    }
}
