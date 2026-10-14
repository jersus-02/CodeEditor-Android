package com.jersus.codeeditor;

import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.text.Editable;
import android.text.InputType;
import android.text.Spanned;
import android.text.TextWatcher;
import android.text.style.ForegroundColorSpan;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.content.ContextCompat;

public class MainActivity extends AppCompatActivity {

    private NativeEditor nativeEditor;
    private EditText codeEditor;
    private TextView lineNumbers;
    private Button btnUndo;
    private Button btnRedo;

    private final Handler debounceHandler = new Handler(Looper.getMainLooper());
    private Runnable debounceRunnable;

    // Guard flag to prevent TextWatcher callbacks while programmatically updating text or applying spans
    private boolean isUpdating = false;

    // Tracks the text currently synchronized with the native TextBuffer
    private String lastSyncedText = "";

    // Syntax highlight colors
    private int colorKeyword;
    private int colorString;
    private int colorComment;
    private int colorNumber;

    // Retain existing native method for backward compatibility
    public native String stringFromJNI();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        nativeEditor = new NativeEditor();

        codeEditor = findViewById(R.id.code_editor);
        lineNumbers = findViewById(R.id.line_numbers);
        btnUndo = findViewById(R.id.btn_undo);
        btnRedo = findViewById(R.id.btn_redo);

        // Configure code editor: no autocorrect/suggestions, no wrapping, horizontal scroll
        codeEditor.setInputType(InputType.TYPE_CLASS_TEXT |
                               InputType.TYPE_TEXT_FLAG_MULTI_LINE |
                               InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS);
        codeEditor.setHorizontallyScrolling(true);

        // Cache syntax colors from resources
        colorKeyword = ContextCompat.getColor(this, R.color.syntax_keyword);
        colorString = ContextCompat.getColor(this, R.color.syntax_string);
        colorComment = ContextCompat.getColor(this, R.color.syntax_comment);
        colorNumber = ContextCompat.getColor(this, R.color.syntax_number);

        // Initial sample code to demonstrate editor and syntax highlighting
        String initialCode =
                "#include <iostream>\n\n" +
                "// Welcome to the Android C++ Code Editor!\n" +
                "int main() {\n" +
                "    int answer = 42;\n" +
                "    double pi = 3.14159;\n" +
                "    std::cout << \"Hello from C++ buffer!\" << std::endl;\n" +
                "    /* Multi-line comments\n" +
                "       are fully supported */\n" +
                "    return 0;\n" +
                "}\n";

        // Initial load uses nativeSetText
        nativeEditor.setText(initialCode);
        lastSyncedText = initialCode;

        isUpdating = true;
        codeEditor.setText(initialCode);
        isUpdating = false;

        applyHighlighting();
        updateLineNumbers();

        debounceRunnable = new Runnable() {
            @Override
            public void run() {
                syncAndHighlight();
            }
        };

        codeEditor.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {}

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {}

            @Override
            public void afterTextChanged(Editable s) {
                if (isUpdating) return;
                debounceHandler.removeCallbacks(debounceRunnable);
                debounceHandler.postDelayed(debounceRunnable, 150);
            }
        });

        btnUndo.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                debounceHandler.removeCallbacks(debounceRunnable);
                flushPendingEdits();
                if (nativeEditor.undo()) {
                    restoreFromNative();
                }
            }
        });

        btnRedo.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                debounceHandler.removeCallbacks(debounceRunnable);
                flushPendingEdits();
                if (nativeEditor.redo()) {
                    restoreFromNative();
                }
            }
        });
    }

    private void syncAndHighlight() {
        if (isUpdating || nativeEditor == null || nativeEditor.isClosed()) return;
        String currentText = codeEditor.getText().toString();
        if (!currentText.equals(lastSyncedText)) {
            computeDiffAndReplace(lastSyncedText, currentText);
            lastSyncedText = currentText;
        }
        applyHighlighting();
        updateLineNumbers();
    }

    private void flushPendingEdits() {
        if (isUpdating || nativeEditor == null || nativeEditor.isClosed()) return;
        String currentText = codeEditor.getText().toString();
        if (!currentText.equals(lastSyncedText)) {
            computeDiffAndReplace(lastSyncedText, currentText);
            lastSyncedText = currentText;
        }
    }

    private void computeDiffAndReplace(String oldText, String newText) {
        int oldLen = oldText.length();
        int newLen = newText.length();

        int prefix = 0;
        while (prefix < oldLen && prefix < newLen && oldText.charAt(prefix) == newText.charAt(prefix)) {
            prefix++;
        }

        int suffix = 0;
        while (suffix < (oldLen - prefix) && suffix < (newLen - prefix) &&
               oldText.charAt(oldLen - 1 - suffix) == newText.charAt(newLen - 1 - suffix)) {
            suffix++;
        }

        int removeLen = oldLen - prefix - suffix;
        String insert = newText.substring(prefix, newLen - suffix);

        nativeEditor.replace(prefix, removeLen, insert);
    }

    private void restoreFromNative() {
        if (nativeEditor == null || nativeEditor.isClosed()) return;
        String restoredText = nativeEditor.getText();
        lastSyncedText = restoredText;

        isUpdating = true;
        try {
            int cursor = codeEditor.getSelectionStart();
            codeEditor.setText(restoredText);
            if (cursor > restoredText.length()) {
                cursor = restoredText.length();
            }
            codeEditor.setSelection(Math.max(0, cursor));
        } finally {
            isUpdating = false;
        }

        applyHighlighting();
        updateLineNumbers();
    }

    private void applyHighlighting() {
        if (nativeEditor == null || nativeEditor.isClosed()) return;
        int[] spans = nativeEditor.highlight();
        Editable editable = codeEditor.getText();
        if (editable == null) return;

        isUpdating = true;
        try {
            ForegroundColorSpan[] oldSpans = editable.getSpans(0, editable.length(), ForegroundColorSpan.class);
            for (ForegroundColorSpan span : oldSpans) {
                editable.removeSpan(span);
            }

            if (spans != null) {
                int textLen = editable.length();
                for (int i = 0; i + 2 < spans.length; i += 3) {
                    int start = spans[i];
                    int end = spans[i + 1];
                    int type = spans[i + 2];

                    if (start >= 0 && end <= textLen && start < end) {
                        int color = getColorForType(type);
                        editable.setSpan(
                            new ForegroundColorSpan(color),
                            start,
                            end,
                            Spanned.SPAN_EXCLUSIVE_EXCLUSIVE
                        );
                    }
                }
            }
        } finally {
            isUpdating = false;
        }
    }

    private int getColorForType(int type) {
        switch (type) {
            case 0: return colorKeyword;
            case 1: return colorString;
            case 2: return colorComment;
            case 3: return colorNumber;
            default: return colorKeyword;
        }
    }

    private void updateLineNumbers() {
        if (nativeEditor == null || nativeEditor.isClosed()) return;
        int lineCount = nativeEditor.getLineCount();
        if (lineCount < 1) lineCount = 1;

        StringBuilder sb = new StringBuilder();
        for (int i = 1; i <= lineCount; i++) {
            if (i > 1) {
                sb.append('\n');
            }
            sb.append(i);
        }
        lineNumbers.setText(sb.toString());
    }

    @Override
    protected void onDestroy() {
        debounceHandler.removeCallbacksAndMessages(null);
        if (nativeEditor != null) {
            nativeEditor.close();
            nativeEditor = null;
        }
        super.onDestroy();
    }
}
