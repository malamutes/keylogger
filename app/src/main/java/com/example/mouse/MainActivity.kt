package com.example.mouse

import android.os.Bundle
import android.util.Log
import android.view.MotionEvent
import android.widget.Button
import androidx.appcompat.app.AppCompatActivity
import java.io.PrintWriter
import java.net.Socket
import java.util.concurrent.Executors
import java.util.concurrent.atomic.AtomicBoolean

class MainActivity : AppCompatActivity() {

    companion object {
        private const val TAG = "RemoteTouchpad"
        private const val HOST = "127.0.0.1"
        private const val PORT = 8080
    }

    private var lastX = 0f
    private var lastY = 0f
    private val ioExecutor = Executors.newSingleThreadExecutor()
    private val isConnecting = AtomicBoolean(false)
    @Volatile
    private var socket: Socket? = null
    @Volatile
    private var out: PrintWriter? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        findViewById<Button>(R.id.connectButton).setOnClickListener {
            ensureConnected(forceReconnect = true)
        }

        findViewById<Button>(R.id.leftButton).setOnClickListener {
            sendCommand("LEFT")
        }

        findViewById<Button>(R.id.rightButton).setOnClickListener {
            sendCommand("RIGHT")
        }

        findViewById<Button>(R.id.dragUpButton).setOnClickListener {
            sendCommand("DRAG_UP")
        }

        findViewById<Button>(R.id.dragDownButton).setOnClickListener {
            sendCommand("DRAG_DOWN")
        }

        findViewById<Button>(R.id.quitButton).setOnClickListener {
            sendCommand("QUIT")
        }

        ensureConnected()
    }

    override fun onResume() {
        super.onResume()
        ensureConnected()
    }

    private fun ensureConnected(forceReconnect: Boolean = false) {
        ioExecutor.execute {
            if (forceReconnect) {
                Log.i(TAG, "Manual reconnect requested")
                closeConnection()
            }

            if (out != null) return@execute
            if (!isConnecting.compareAndSet(false, true)) return@execute

            try {
                while (!isFinishing && out == null) {
                    try {
                        val newSocket = Socket(HOST, PORT)
                        socket = newSocket
                        out = PrintWriter(newSocket.getOutputStream(), true)
                        Log.i(TAG, "Connected to $HOST:$PORT")
                    } catch (e: Exception) {
                        Log.w(TAG, "Connect failed. Retrying...", e)
                        Thread.sleep(700)
                    }
                }
            } finally {
                isConnecting.set(false)
            }
        }
    }

    override fun onTouchEvent(event: MotionEvent): Boolean {
        when (event.action) {
            MotionEvent.ACTION_DOWN -> {
                lastX = event.x
                lastY = event.y
            }

            MotionEvent.ACTION_MOVE -> {
                val dx = event.x - lastX
                val dy = event.y - lastY

                sendMove(dx, dy)

                lastX = event.x
                lastY = event.y
            }
        }
        return true
    }

    private fun sendMove(dx: Float, dy: Float) {
        sendCommand("MOVE $dx $dy")
    }

    private fun sendCommand(command: String) {
        ioExecutor.execute {
            val writer = out
            if (writer == null) {
                ensureConnected()
                return@execute
            }

            try {
                writer.println(command)
                if (writer.checkError()) {
                    Log.w(TAG, "Socket write failed; reconnecting")
                    closeConnection()
                    ensureConnected()
                }
            } catch (e: Exception) {
                Log.w(TAG, "Send failed; reconnecting", e)
                closeConnection()
                ensureConnected()
            }
        }
    }

    private fun closeConnection() {
        out = null
        try {
            socket?.close()
        } catch (_: Exception) {
        }
        socket = null
    }

    override fun onDestroy() {
        closeConnection()
        ioExecutor.shutdownNow()
        super.onDestroy()
    }
}
