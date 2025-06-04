package com.bacorp.soundmirror.data

import java.net.InetAddress
import java.net.NetworkInterface

/**
 * A small utility that tries to determine the “common network part”
 * of this device’s IP, e.g. “192.168.1.” so the user can quickly choose an address.
 */
class NetworkHelper() {
    fun getCommonNetworkPart(): String? {
        try {
            val interfaces = NetworkInterface.getNetworkInterfaces()
            while (interfaces.hasMoreElements()) {
                val nic = interfaces.nextElement()
                if (!nic.isUp || nic.isLoopback) continue
                for (addr in nic.inetAddresses) {
                    if (addr is InetAddress && !addr.isLoopbackAddress && addr.address.size == 4) {
                        val hostAddr = addr.hostAddress
                        val parts = hostAddr?.split(".")
                        if (parts?.size == 4) {
                            return "${parts[0]}.${parts[1]}.${parts[2]}."
                        }
                    }
                }
            }
        } catch (_: Exception) {}
        return null
    }
}
