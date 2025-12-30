package com.bacorp.soundmirror.data

import androidx.datastore.preferences.core.stringPreferencesKey

object AppPreferencesConstants {
    const val DATASTORE_NAME = "app_preferences"
    val IP_ADDRESS_KEY = stringPreferencesKey("saved_ip_address")
}
