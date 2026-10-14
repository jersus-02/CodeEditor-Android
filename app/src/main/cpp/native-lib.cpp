#include <jni.h>
#include <string>
#include <vector>
#include <cstdint>
#include "text_buffer.h"
#include "highlighter.h"

static std::string utf16ToUtf8(const jchar* utf16, jsize len) {
    std::string utf8;
    utf8.reserve(len);
    for (jsize i = 0; i < len; ++i) {
        uint32_t cp = utf16[i];
        if (cp >= 0xD800 && cp <= 0xDBFF && i + 1 < len) {
            uint32_t high = cp;
            uint32_t low = utf16[i + 1];
            if (low >= 0xDC00 && low <= 0xDFFF) {
                cp = 0x10000 + ((high - 0xD800) << 10) + (low - 0xDC00);
                ++i;
            }
        }
        if (cp <= 0x7F) {
            utf8.push_back(static_cast<char>(cp));
        } else if (cp <= 0x7FF) {
            utf8.push_back(static_cast<char>(0xC0 | ((cp >> 6) & 0x1F)));
            utf8.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else if (cp <= 0xFFFF) {
            utf8.push_back(static_cast<char>(0xE0 | ((cp >> 12) & 0x0F)));
            utf8.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            utf8.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else {
            utf8.push_back(static_cast<char>(0xF0 | ((cp >> 18) & 0x07)));
            utf8.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
            utf8.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            utf8.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
    }
    return utf8;
}

static jstring utf8ToJstring(JNIEnv* env, const std::string& utf8) {
    std::vector<jchar> utf16;
    utf16.reserve(utf8.size());
    size_t i = 0;
    const size_t n = utf8.size();

    while (i < n) {
        uint32_t cp = 0;
        unsigned char c = static_cast<unsigned char>(utf8[i]);
        if (c <= 0x7F) {
            cp = c;
            i += 1;
        } else if ((c & 0xE0) == 0xC0) {
            if (i + 1 < n) {
                cp = ((c & 0x1F) << 6) | (static_cast<unsigned char>(utf8[i + 1]) & 0x3F);
                i += 2;
            } else {
                i += 1;
                cp = 0xFFFD;
            }
        } else if ((c & 0xF0) == 0xE0) {
            if (i + 2 < n) {
                cp = ((c & 0x0F) << 12) | ((static_cast<unsigned char>(utf8[i + 1]) & 0x3F) << 6)
                                       | (static_cast<unsigned char>(utf8[i + 2]) & 0x3F);
                i += 3;
            } else {
                i += 1;
                cp = 0xFFFD;
            }
        } else if ((c & 0xF8) == 0xF0) {
            if (i + 3 < n) {
                cp = ((c & 0x07) << 18) | ((static_cast<unsigned char>(utf8[i + 1]) & 0x3F) << 12)
                                       | ((static_cast<unsigned char>(utf8[i + 2]) & 0x3F) << 6)
                                       | (static_cast<unsigned char>(utf8[i + 3]) & 0x3F);
                i += 4;
            } else {
                i += 1;
                cp = 0xFFFD;
            }
        } else {
            i += 1;
            cp = 0xFFFD;
        }

        if (cp <= 0xFFFF) {
            utf16.push_back(static_cast<jchar>(cp));
        } else {
            cp -= 0x10000;
            utf16.push_back(static_cast<jchar>(0xD800 + (cp >> 10)));
            utf16.push_back(static_cast<jchar>(0xDC00 + (cp & 0x3FF)));
        }
    }
    return env->NewString(utf16.data(), static_cast<jsize>(utf16.size()));
}

extern "C" {

JNIEXPORT jlong JNICALL
Java_com_jersus_codeeditor_NativeEditor_nativeCreate(JNIEnv* /* env */, jclass /* clazz */) {
    TextBuffer* buffer = new TextBuffer();
    return reinterpret_cast<jlong>(buffer);
}

JNIEXPORT void JNICALL
Java_com_jersus_codeeditor_NativeEditor_nativeDestroy(JNIEnv* /* env */, jclass /* clazz */, jlong handle) {
    TextBuffer* buffer = reinterpret_cast<TextBuffer*>(handle);
    delete buffer;
}

JNIEXPORT void JNICALL
Java_com_jersus_codeeditor_NativeEditor_nativeSetText(JNIEnv* env, jclass /* clazz */, jlong handle, jstring s) {
    TextBuffer* buffer = reinterpret_cast<TextBuffer*>(handle);
    if (!buffer) return;
    if (!s) {
        buffer->setText("");
        return;
    }
    const jchar* chars = env->GetStringChars(s, nullptr);
    jsize len = env->GetStringLength(s);
    std::string utf8 = utf16ToUtf8(chars, len);
    env->ReleaseStringChars(s, chars);
    buffer->setText(utf8);
}

JNIEXPORT jstring JNICALL
Java_com_jersus_codeeditor_NativeEditor_nativeGetText(JNIEnv* env, jclass /* clazz */, jlong handle) {
    TextBuffer* buffer = reinterpret_cast<TextBuffer*>(handle);
    if (!buffer) return env->NewStringUTF("");
    return utf8ToJstring(env, buffer->getText());
}

JNIEXPORT void JNICALL
Java_com_jersus_codeeditor_NativeEditor_nativeReplace(JNIEnv* env, jclass /* clazz */, jlong handle,
                                                      jint start, jint removeLen, jstring insert) {
    TextBuffer* buffer = reinterpret_cast<TextBuffer*>(handle);
    if (!buffer) return;
    std::string insertUtf8;
    if (insert) {
        const jchar* chars = env->GetStringChars(insert, nullptr);
        jsize len = env->GetStringLength(insert);
        insertUtf8 = utf16ToUtf8(chars, len);
        env->ReleaseStringChars(insert, chars);
    }
    size_t u16_start = (start >= 0) ? static_cast<size_t>(start) : 0;
    size_t u16_remove = (removeLen >= 0) ? static_cast<size_t>(removeLen) : 0;
    buffer->replaceUtf16(u16_start, u16_remove, insertUtf8);
}

JNIEXPORT jboolean JNICALL
Java_com_jersus_codeeditor_NativeEditor_nativeUndo(JNIEnv* /* env */, jclass /* clazz */, jlong handle) {
    TextBuffer* buffer = reinterpret_cast<TextBuffer*>(handle);
    if (!buffer) return JNI_FALSE;
    return buffer->undo() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_jersus_codeeditor_NativeEditor_nativeRedo(JNIEnv* /* env */, jclass /* clazz */, jlong handle) {
    TextBuffer* buffer = reinterpret_cast<TextBuffer*>(handle);
    if (!buffer) return JNI_FALSE;
    return buffer->redo() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jint JNICALL
Java_com_jersus_codeeditor_NativeEditor_nativeLineCount(JNIEnv* /* env */, jclass /* clazz */, jlong handle) {
    TextBuffer* buffer = reinterpret_cast<TextBuffer*>(handle);
    if (!buffer) return 1;
    return static_cast<jint>(buffer->lineCount());
}

JNIEXPORT jintArray JNICALL
Java_com_jersus_codeeditor_NativeEditor_nativeHighlight(JNIEnv* env, jclass /* clazz */, jlong handle) {
    TextBuffer* buffer = reinterpret_cast<TextBuffer*>(handle);
    if (!buffer) {
        return env->NewIntArray(0);
    }
    std::vector<HighlightSpan> spans = highlightCode(buffer->getText());
    jsize numElements = static_cast<jsize>(spans.size() * 3);
    jintArray result = env->NewIntArray(numElements);
    if (!result) return nullptr;

    if (numElements > 0) {
        std::vector<jint> raw(numElements);
        for (size_t i = 0; i < spans.size(); ++i) {
            raw[3 * i]     = static_cast<jint>(spans[i].start);
            raw[3 * i + 1] = static_cast<jint>(spans[i].end);
            raw[3 * i + 2] = static_cast<jint>(spans[i].type);
        }
        env->SetIntArrayRegion(result, 0, numElements, raw.data());
    }
    return result;
}

JNIEXPORT jstring JNICALL
Java_com_jersus_codeeditor_MainActivity_stringFromJNI(JNIEnv* env, jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}

} // extern "C"
