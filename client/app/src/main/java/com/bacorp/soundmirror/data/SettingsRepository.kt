package com.bacorp.soundmirror.data

import android.content.Context
import androidx.datastore.preferences.core.edit
import androidx.datastore.preferences.core.stringPreferencesKey
import androidx.datastore.preferences.preferencesDataStore
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.map

private const val DATASTORE_NAME = "app_preferences"
private val Context.dataStore by preferencesDataStore(DATASTORE_NAME)

class SettingsRepository(private val context: Context) {
    companion object {
        private val IP_ADDRESS_KEY = stringPreferencesKey("saved_ip_address")
    }

    fun observeIpAddress(): Flow<String> =
        context.dataStore.data
            .map { prefs -> prefs[IP_ADDRESS_KEY] ?: "" }

    suspend fun saveIpAddress(ip: String) {
        context.dataStore.edit { prefs ->
            prefs[IP_ADDRESS_KEY] = ip
        }
    }

    fun isValidIpAddress(ip: String): Boolean {
        val regex =
            Regex("""^((25[0-5]|(2[0-4]|1\d|[1-9]|)\d)\.?\b){4}$""")
        return regex.matches(ip)
    }
}
