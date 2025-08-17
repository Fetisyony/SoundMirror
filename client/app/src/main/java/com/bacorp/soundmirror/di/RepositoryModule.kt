package com.bacorp.soundmirror.di

import androidx.datastore.core.DataStore
import androidx.datastore.preferences.core.Preferences
import com.bacorp.soundmirror.data.PreferencesRepositoryImpl
import com.bacorp.soundmirror.domain.PreferencesRepository
import dagger.Module
import dagger.Provides
import dagger.hilt.InstallIn
import dagger.hilt.android.components.ViewModelComponent

@Module
@InstallIn(ViewModelComponent::class)
object RepositoryModule {
    @Provides
    fun providesPreferencesRepository(dataStore: DataStore<Preferences>): PreferencesRepository {
        return PreferencesRepositoryImpl(dataStore)
    }
}
