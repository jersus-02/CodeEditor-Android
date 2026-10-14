# CodeEditor-Android

An efficient code editor for Android featuring a native C++ core and a thin Java UI.

## Architecture Overview

The editor is built with a decoupled architecture separating UI presentation from editing and syntax logic:

```
┌────────────────────────────────────────────────────────┐
│                        Java UI                         │
│  MainActivity (EditText, Line Numbers, Undo/Redo UI)   │
│                           │                            │
│                   NativeEditor (Java)                  │
└───────────────────────────┬────────────────────────────┘
                            │ JNI
┌───────────────────────────▼────────────────────────────┐
│                    native-lib.cpp                      │
│             (JNI bridge & string marshaling)           │
├────────────────────────────┬───────────────────────────┤
│         TextBuffer         │        Highlighter        │
│  - UTF-8 text storage      │  - C++/Java lexer         │
│  - Undo/redo operation     │  - UTF-16 span mapping    │
│    history stack           │  - Token classifications: │
│  - UTF-16 <-> UTF-8 index  │    Keywords, Strings,     │
│    translation             │    Comments, Numbers      │
└────────────────────────────┴───────────────────────────┘
```

### Components

1. **Java UI (`MainActivity.java` & `NativeEditor.java`)**:
   - **UI Hierarchy**: A dark-themed layout containing a toolbar with Undo and Redo buttons, a line-number gutter `TextView` on the left, and a monospace `EditText` configured with no auto-suggestions and horizontal scrolling inside a vertical `ScrollView`.
   - **Debounced Sync**: User typing is captured via `TextWatcher` and debounced (~150ms) using a `Handler`. Instead of replacing the entire buffer on each keystroke, Java computes the minimal diff (common prefix and suffix) and calls `nativeReplace`, allowing C++ to record fine-grained undo operations.
   - **Spans**: Highlighting spans (`ForegroundColorSpan`) are applied to the `Editable` text buffer without re-triggering edit listeners using a synchronization guard flag.
   - **`NativeEditor`**: An `AutoCloseable` wrapper managing the native `long` handle pointer (`TextBuffer*`), safe lifecycle teardown (`nativeDestroy`), and static native method bindings.

2. **JNI Bridge (`native-lib.cpp`)**:
   - Marshals between Android JVM strings and C++ UTF-8 strings using `GetStringChars` / `ReleaseStringChars` and manual surrogate pair / UTF-8 conversion to avoid modified-UTF-8 edge cases.
   - Exposes handle-based operations (`nativeCreate`, `nativeDestroy`, `nativeSetText`, `nativeGetText`, `nativeReplace`, `nativeUndo`, `nativeRedo`, `nativeLineCount`, `nativeHighlight`).

3. **C++ Text Buffer (`text_buffer.h` / `text_buffer.cpp`)**:
   - Manages text in standard UTF-8 (`std::string`).
   - Maintains undo and redo stacks storing `EditOperation { byte_pos, old_text, new_text }`.
   - Converts Java UTF-16 code unit offsets to UTF-8 byte offsets via `replaceUtf16()` so edits originating from Android's UTF-16 `EditText` map accurately even with multi-byte or emoji characters.
   - Computes line counts for the line number gutter.

4. **C++ Highlighter (`highlighter.h` / `highlighter.cpp`)**:
   - Lexes C++ and Java syntax:
     - `0`: Keywords (`class`, `public`, `int`, `return`, `auto`, etc.)
     - `1`: String and character literals (`"..."`, `'...'`)
     - `2`: Line and block comments (`// ...`, `/* ... */`)
     - `3`: Number literals (hex, binary, float, integers, exponents)
   - Accurately converts token UTF-8 byte bounds into UTF-16 code unit spans required by Android's `Spannable` string index system.
