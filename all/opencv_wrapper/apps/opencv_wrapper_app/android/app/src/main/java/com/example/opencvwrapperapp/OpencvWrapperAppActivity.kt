package com.example.opencvwrapperapp

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.widget.Button
import android.widget.EditText
import com.example.opencvwrapper.OpencvWrapper

class OpencvWrapperAppActivity : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        val rsaButton: Button = findViewById(R.id.button)
        rsaButton.setOnClickListener {
            val edit: EditText = findViewById(R.id.edit)
            val opencvWrapper = OpencvWrapper()
            edit.setText(opencvWrapper.getVersion())
        }
    }
}