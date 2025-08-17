package com.bacorp.soundmirror.data

import android.content.Context
import androidx.datastore.preferences.preferencesDataStore
import com.bacorp.soundmirror.data.AppPreferencesConstants.DATASTORE_NAME

val Context.appPreferencesDataStore by preferencesDataStore(
    name = DATASTORE_NAME
)

