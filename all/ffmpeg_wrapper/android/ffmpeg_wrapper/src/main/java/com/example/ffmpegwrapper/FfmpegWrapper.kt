package com.example.ffmpegwrapper

import android.util.Log

private val TAG = FfmpegWrapper::class.java.simpleName

class FfmpegWrapper {
    fun print() {
        Log.v(TAG, getVersion())
        Log.v(TAG, getOsName())
        Log.v(TAG, getName())
    }

    /**
     * A native method that is implemented by the 'ffmpeg_wrapper' native library,
     * which is packaged with this application.
     */
    external fun getVersion(): String
    external fun getOsName(): String
    external fun getName(): String

    companion object {
        // Used to load the 'ffmpeg_wrapper' library on application startup.
        init {
            System.loadLibrary("ffmpeg_wrapper")
        }
    }
}