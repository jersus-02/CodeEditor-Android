package com.jaag_projects.codeeditor;

import androidx.appcompat.app.AppCompatActivity;
import android.os.Bundle;

public class MainActivity extends AppCompatActivity {

    // Load the C++ library compiled by CMake
    static {
        System.loadLibrary("codeeditor-native");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
      super.onCreate(savedInstanceState);
      setContentView(R.layout.activity_main);
      int errorcode  = Main();
    }

    // Native C++ method declaration
    public native int Main();
}
