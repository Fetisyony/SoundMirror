package com.bacorp.soundmirror.domain

import kotlinx.coroutines.flow.Flow


interface PreferencesRepository {
    fun observeIpAddress(): Flow<String>

    suspend fun saveIpAddress(ip: String)
}
