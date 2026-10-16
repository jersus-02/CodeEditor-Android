package com.jaag_projects.codeeditor;

import androidx.appcompat.app.AppCompatActivity;
import android.os.Bundle;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.os.Environment;
import android.provider.Settings;

public class MainActivity extends AppCompatActivity {

	static {
		System.loadLibrary("codeeditor-native");
	}

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
        	int errorcode = Main();
        	requestStoragePermission();
	}

	private void requestStoragePermission() {
		// Android 11 (API 30) through Android 13 (API 33) and higher
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
			if (!Environment.isExternalStorageManager()) {
				Intent intent = new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION);
				Uri uri = Uri.fromParts("package", getPackageName(), null);
				intent.setData(uri);
				startActivity(intent);
			}
		}
	}
    // Native C++ method declaration
    public native int Main();
}
