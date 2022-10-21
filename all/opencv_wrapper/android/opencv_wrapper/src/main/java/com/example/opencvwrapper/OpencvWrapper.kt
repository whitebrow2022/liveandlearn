package com.example.opencvwrapper

import android.util.Log

private val TAG = OpencvWrapper::class.java.simpleName

class OpencvWrapper {
    fun print() {
        Log.v(TAG, getVersion())
        Log.v(TAG, getOsName())
        Log.v(TAG, getName())
    }

    /**
     * A native method that is implemented by the 'opencv_wrapper' native library,
     * which is packaged with this application.
     */
    external fun getVersion(): String
    external fun getOsName(): String
    external fun getName(): String

    companion object {
        // Used to load the 'opencv_wrapper' library on application startup.
        init {
            System.loadLibrary("opencv_wrapper")
        }
    }
}