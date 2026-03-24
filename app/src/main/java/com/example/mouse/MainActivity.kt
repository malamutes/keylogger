package com.example.mouse

import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.util.Log
import android.view.GestureDetector
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
        private const val SCROLL_REPEAT_MS = 80L
    }

    private var lastX = 0f
    private var lastY = 0f
    private val ioExecutor = Executors.newSingleThreadExecutor()
    private val isConnecting = AtomicBoolean(false)
    private lateinit var gestureDetector: GestureDetector
    private var isDragDownMode = false
    private val uiHandler = Handler(Looper.getMainLooper())
    private val scrollRepeaters = mutableMapOf<Int, Runnable>()
    @Volatile
    private var socket: Socket? = null
    @Volatile
    private var out: PrintWriter? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        gestureDetector = GestureDetector(this, object : GestureDetector.SimpleOnGestureListener() {
            override fun onDown(e: MotionEvent): Boolean = true

            override fun onSingleTapConfirmed(e: MotionEvent): Boolean {
                val halfScreen = resources.displayMetrics.widthPixels / 2f
                if (e.x < halfScreen) {
                    sendCommand("LEFT")
                } else {
                    sendCommand("RIGHT")
                }
                return true
            }

            override fun onDoubleTapEvent(e: MotionEvent): Boolean {
                if (e.actionMasked != MotionEvent.ACTION_DOWN) return false

                val halfScreen = resources.displayMetrics.widthPixels / 2f
                if (e.x < halfScreen && !isDragDownMode) {
                    isDragDownMode = true
                    sendCommand("DRAG_DOWN")
                    return true
                }
                return false
            }
        })

        findViewById<Button>(R.id.connectButton).setOnClickListener {
            ensureConnected(forceReconnect = true)
        }

        setupRepeatingScrollArea(R.id.scrollUpArea, "SCROLL_UP")
        setupRepeatingScrollArea(R.id.scrollDownArea, "SCROLL_DOWN")

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
        gestureDetector.onTouchEvent(event)

        when (event.actionMasked) {
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

            MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> {
                if (isDragDownMode) {
                    isDragDownMode = false
                    sendCommand("DRAG_UP")
                }
            }
        }
        return true
    }

    private fun sendMove(dx: Float, dy: Float) {
        sendCommand("MOVE $dx $dy")
    }

    private fun setupRepeatingScrollArea(viewId: Int, command: String) {
        findViewById<Button>(viewId).setOnTouchListener { _, event ->
            when (event.actionMasked) {
                MotionEvent.ACTION_DOWN -> {
                    startRepeatingScroll(viewId, command)
                    true
                }

                MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> {
                    stopRepeatingScroll(viewId)
                    true
                }

                else -> false
            }
        }
    }

    private fun startRepeatingScroll(viewId: Int, command: String) {
        if (scrollRepeaters.containsKey(viewId)) return

        val repeater = object : Runnable {
            override fun run() {
                sendCommand(command)
                uiHandler.postDelayed(this, SCROLL_REPEAT_MS)
            }
        }
        scrollRepeaters[viewId] = repeater
        repeater.run()
    }

    private fun stopRepeatingScroll(viewId: Int) {
        val repeater = scrollRepeaters.remove(viewId) ?: return
        uiHandler.removeCallbacks(repeater)
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
        scrollRepeaters.values.forEach { uiHandler.removeCallbacks(it) }
        scrollRepeaters.clear()
        closeConnection()
        ioExecutor.shutdownNow()
        super.onDestroy()
    }
}
