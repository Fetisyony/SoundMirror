package com.bacorp.soundmirror.data

import androidx.datastore.core.DataStore
import androidx.datastore.preferences.core.Preferences
import androidx.datastore.preferences.core.edit
import com.bacorp.soundmirror.data.AppPreferencesConstants.IP_ADDRESS_KEY
import com.bacorp.soundmirror.domain.PreferencesRepository
import jakarta.inject.Inject
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.map

class PreferencesRepositoryImpl @Inject constructor(
    private val dataStore: DataStore<Preferences>
) : PreferencesRepository {
    override fun observeIpAddress(): Flow<String> =
        dataStore.data
            .map { prefs -> prefs[IP_ADDRESS_KEY] ?: "" }

    override suspend fun saveIpAddress(ip: String) {
        dataStore.edit { prefs ->
            prefs[IP_ADDRESS_KEY] = ip
        }
    }
}
