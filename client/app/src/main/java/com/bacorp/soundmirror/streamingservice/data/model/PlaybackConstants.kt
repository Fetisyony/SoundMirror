package com.bacorp.soundmirror.streamingservice.data.model

object PlaybackConstants {
    const val PORT_STREAMING = 20000
    const val PORT_SYNC = 20001
    const val CONTROL_PORT = 20002

    const val TIMEOUT_MILLIS = 5000
    const val UDP_TIMEOUT_MILLIS = 0

    const val FORMAT_PACKET_SIZE = 6
    const val MESSAGE_HEADER_SIZE = 4
    const val TIMESTAMP_SIZE = 8
}