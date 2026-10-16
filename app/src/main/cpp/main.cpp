#include <jni.h>
#include <string>
#include <iostream>

extern "C" JNIEXPORT jint JNICALL
Java_com_jaag_1projects_codeeditor_MainActivity_Main( JNIEnv* env, jobject me,
        jclass textview, jobjectArray IDs
) {
        auto& ENV = *env;
        jmethodID setText = ENV.GetMethodID(textview, "setText", "(Ljava/lang/CharSequence;)V");
        ENV.CallVoidMethod(ENV.GetObjectArrayElement(IDs,0), setText, EMV.NewStringUTF("hello"));
        return 0;
}
