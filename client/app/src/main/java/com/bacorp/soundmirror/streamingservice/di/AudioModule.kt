package com.bacorp.soundmirror.streamingservice.di

import android.content.Context
import dagger.Module
import dagger.Provides
import dagger.hilt.InstallIn
import dagger.hilt.components.SingletonComponent
import dagger.hilt.android.qualifiers.ApplicationContext
import javax.inject.Qualifier

@Qualifier
@Retention(AnnotationRetention.BINARY)
annotation class PlcModelBytes

@Module
@InstallIn(SingletonComponent::class)
object AudioModule {
    @Provides
    @PlcModelBytes
    fun providePlcModelBytes(@ApplicationContext context: Context): ByteArray {
        return context.assets.open("tPLCnet_medium.onnx").use { inputStream ->
            inputStream.readBytes()
        }
    }
}
