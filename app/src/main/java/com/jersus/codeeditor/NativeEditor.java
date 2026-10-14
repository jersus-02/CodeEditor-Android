package com.jersus.codeeditor;

import java.io.Closeable;

public class NativeEditor implements Closeable {

    static {
        System.loadLibrary("codeeditor-native");
    }

    private long handle;

    public NativeEditor() {
        handle = nativeCreate();
    }

    private void checkHandle() {
        if (handle == 0) {
            throw new IllegalStateException("NativeEditor has been closed");
        }
    }

    public synchronized void setText(String text) {
        checkHandle();
        nativeSetText(handle, text);
    }

    public synchronized String getText() {
        checkHandle();
        return nativeGetText(handle);
    }

    public synchronized void replace(int start, int removeLen, String insert) {
        checkHandle();
        nativeReplace(handle, start, removeLen, insert);
    }

    public synchronized boolean undo() {
        checkHandle();
        return nativeUndo(handle);
    }

    public synchronized boolean redo() {
        checkHandle();
        return nativeRedo(handle);
    }

    public synchronized int getLineCount() {
        checkHandle();
        return nativeLineCount(handle);
    }

    public synchronized int[] highlight() {
        checkHandle();
        return nativeHighlight(handle);
    }

    public synchronized boolean isClosed() {
        return handle == 0;
    }

    @Override
    public synchronized void close() {
        if (handle != 0) {
            nativeDestroy(handle);
            handle = 0;
        }
    }

    // Native JNI methods matching Java_com_jersus_codeeditor_NativeEditor_<method>
    public static native long nativeCreate();
    public static native void nativeDestroy(long h);
    public static native void nativeSetText(long h, String s);
    public static native String nativeGetText(long h);
    public static native void nativeReplace(long h, int start, int removeLen, String insert);
    public static native boolean nativeUndo(long h);
    public static native boolean nativeRedo(long h);
    public static native int nativeLineCount(long h);
    public static native int[] nativeHighlight(long h);
}
