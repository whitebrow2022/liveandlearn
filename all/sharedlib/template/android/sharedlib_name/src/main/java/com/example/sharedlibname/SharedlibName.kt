package com.example.sharedlibname

import android.util.Log

private val TAG = SharedlibName::class.java.simpleName

class SharedlibName {
    fun print() {
        Log.v(TAG, getVersion())
        Log.v(TAG, getOsName())
        Log.v(TAG, getName())
    }

    /**
     * A native method that is implemented by the 'sharedlib_name' native library,
     * which is packaged with this application.
     */
    external fun getVersion(): String
    external fun getOsName(): String
    external fun getName(): String

    companion object {
        // Used to load the 'sharedlib_name' library on application startup.
        init {
            System.loadLibrary("sharedlib_name")
        }
    }
}