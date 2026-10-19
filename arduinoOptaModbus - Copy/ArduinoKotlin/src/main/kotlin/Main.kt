package arduino

import org.apache.plc4x.java.api.PlcDriverManager
import org.apache.plc4x.java.api.types.PlcResponseCode
import java.util.concurrent.TimeUnit

operator fun String.times(n: Int): String = this.repeat(n)

fun main() {
    val connectionUrl = "modbus-tcp://10.0.0.1:502?unit-identifier=1"
    val connectionManager = PlcDriverManager.getDefault().connectionManager

    println("Connecting to $connectionUrl...")

    try {
        connectionManager.getConnection(connectionUrl).use { connection ->

            val readRequest = connection.readRequestBuilder()
                .addTagAddress("uptime", "holding-register:1:UINT")
                .addTagAddress("ledIndex", "holding-register:2:UINT")
                .addTagAddress("sampleCounter", "holding-register:3:UINT")
                .addTagAddress("led0", "coil:1")
                .addTagAddress("led1", "coil:2")
                .addTagAddress("led2", "coil:3")
                .addTagAddress("led3", "coil:4")
                .build()

            fun formatLed(state: Boolean): String = if (state) "ON" else "OFF"

            while (true) {
                try {
                    if (!connection.isConnected) {
                        connection.connect()
                    }

                    val response = readRequest.execute().get(3, TimeUnit.SECONDS)

                    if (response.getResponseCode("uptime") == PlcResponseCode.OK) {
                        val uptime = response.getInteger("uptime")
                        val ledIndex = response.getInteger("ledIndex")
                        val sampleCounter = response.getInteger("sampleCounter")

                        val led0 = response.getBoolean("led0")
                        val led1 = response.getBoolean("led1")
                        val led2 = response.getBoolean("led2")
                        val led3 = response.getBoolean("led3")

                        println("Uptime: ${uptime}s | LedIdx: $ledIndex | Samples: $sampleCounter ")
                        println("D0: ${formatLed(led0)} | D1: ${formatLed(led1)} | D2: ${formatLed(led2)} | D3: ${formatLed(led3)}")
                        println("=" * 60)
                    } else {
                        println("Read response error: ${response.getResponseCode("uptime")}")
                    }
                } catch (e: Exception) {
                    println("Error during read: ${e.message}")
                }

                Thread.sleep(1000)
            }
        }
    } catch (e: Exception) {
        println("Modbus connection error: ${e.message}")
    }
}
