package com.jaag_projects.codeeditor;

import androidx.appcompat.app.AppCompatActivity;
import android.os.Bundle;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.os.Environment;
import android.provider.Settings;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity {

	static {
		System.loadLibrary("codeeditor-native");
	}

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		int errorCode = Main(TextView.class,findViewById(R.id.my_text_view));
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
	public native int Main(Class<?> textClass, TextView ui);
}
