#include <jni.h>
#include <string>
#include <iostream>

extern "C" JNIEXPORT jint JNICALL
Java_com_jaag_1projects_codeeditor_MainActivity_Main( JNIEnv* env, jobject me,
        jclass textview, jobject ID
) {
        jmethodID setText = env->GetMethodID(textview, "setText", "(Ljava/lang/CharSequence;)V");
        env->CallVoidMethod(ID, setText, env->NewStringUTF("hello"));
        return 0;
}
