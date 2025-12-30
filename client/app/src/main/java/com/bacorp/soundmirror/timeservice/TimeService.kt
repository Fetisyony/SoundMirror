package com.bacorp.soundmirror.timeservice

import android.util.Log
import com.bacorp.soundmirror.timeservice.TimeServiceConstants.TIMEOUT
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import java.io.DataInputStream
import java.io.DataOutputStream
import java.net.Socket
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.nio.charset.StandardCharsets
import java.time.Instant
import kotlin.properties.Delegates

class TimeService {
    private lateinit var serverIp: String
    private var serverPort by Delegates.notNull<Int>()

    @Volatile
    private var timeOffsetMs: Long = 0L

    fun initialize(ip: String, port: Int) {
        this.serverIp = ip
        this.serverPort = port
    }

    suspend fun sync() = withContext(Dispatchers.IO) {
        Socket(serverIp, serverPort).use { socket ->
            socket.soTimeout = TIMEOUT

            val dos = DataOutputStream(socket.getOutputStream())
            val dis = DataInputStream(socket.getInputStream())

            val clientDepartureTime = getServerTimeMillis()
            dos.write("SYNC".toByteArray(StandardCharsets.US_ASCII))
            dos.flush()

            val responseBuffer = ByteArray(16)
            dis.readFully(responseBuffer)

            val clientArrivalTime = getServerTimeMillis()

            val byteBuffer = ByteBuffer.wrap(responseBuffer).order(ByteOrder.LITTLE_ENDIAN)

            val serverArrivalTime = byteBuffer.long
            val serverDepartureTime = byteBuffer.long

            val roundTripDelay = (clientArrivalTime - clientDepartureTime) - (serverDepartureTime - serverArrivalTime)
            val estimatedOneWayLatency = roundTripDelay / 2

            val serverTimeWhenClientReceived = serverDepartureTime + estimatedOneWayLatency
            val calculatedOffset = serverTimeWhenClientReceived - clientArrivalTime

            timeOffsetMs = calculatedOffset

            Log.d("SYNC", "clientDepartureTime (t1): $clientDepartureTime")
            Log.d("SYNC", "serverArrivalTime (t2): $serverArrivalTime")
            Log.d("SYNC", "serverDepartureTime (t3): $serverDepartureTime")
            Log.d("SYNC", "clientArrivalTime (t4): $clientArrivalTime")

            Log.d("SYNC", "Time synchronization successful!")
            Log.d("SYNC",   "Calculated Round-Trip Delay: $roundTripDelay ms")
            Log.d("SYNC",   "Estimated One-Way Latency: $estimatedOneWayLatency ms")
            Log.d("SYNC",   "Calculated Offset (add to client time): $calculatedOffset ms")
        }
    }

    fun getServerTimeMillis(): Long {
        return Instant.now().toEpochMilli() + timeOffsetMs
    }
}
