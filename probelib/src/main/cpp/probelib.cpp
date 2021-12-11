#include <jni.h>
#include <string>
#include <android/log.h>

#include "jvmti.h"


#define LOG_TAG "jvmti"

#define ALOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#define ALOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define ALOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)


/*JNIEXPORT void extern "C" JNICALL Java_com_hana_detection_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject *//* this *//*) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}*/

// https://www.oracle.com/technical-resources/articles/javase/jvm-tool-interface.html
// https://developer.android.com/training/articles/perf-jni#local-and-global-references

// https://android.googlesource.com/platform/tools/dexter/ dexer用来操作dex文件，可做加解密等

static jrawMonitorID gb_lock;

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }
    ALOGI("==============JNI_OnLoad====================");
    return JNI_VERSION_1_6;
}

jvmtiEnv *CreateJvmtiEnv(JavaVM *vm) {
    jvmtiEnv *jvmti_env;
    jint result = vm->GetEnv((void **) &jvmti_env, JVMTI_VERSION_1_2);
    if (result != JNI_OK) {
        return nullptr;
    }

    return jvmti_env;
}

void printStackTrace(JNIEnv *env, jobject exception) {
    jclass throwable_class = (*env).FindClass("java/lang/Throwable");
    jmethodID print_method = (*env).GetMethodID(throwable_class, "printStackTrace", "()V");
    (*env).CallVoidMethod(exception, print_method);
}

void printInvokeStack(jvmtiEnv *jvmti, JNIEnv *env,
                      jthread thr, jmethodID method) {
    char *name;
    char *signature;
    char *generic;

    char *signatureClz;
    char *genericClz;

    jclass clazz;

    auto err = (*jvmti).GetMethodName(method, &name, &signature, &generic);
    auto errDeclaringClass = jvmti->GetMethodDeclaringClass(method, &clazz);
    // GetMethodName success JVMTI! method is setDisplayListProperties, Landroid/view/View;,(null)
    auto errClass = (*jvmti).GetClassSignature(clazz, &signatureClz,
                                               &genericClz);
    if (err == JVMTI_ERROR_NONE && errClass == JVMTI_ERROR_NONE &&
        errDeclaringClass == JVMTI_ERROR_NONE) {
        ALOGI("GetMethodName success JVMTI! method is %1$s, %2$s,%3$s", name, signatureClz,
              genericClz);
    }
    jvmti->Deallocate((unsigned char*)name);
    jvmti->Deallocate((unsigned char*)signature);
    jvmti->Deallocate((unsigned char*)generic);
    jvmti->Deallocate((unsigned char*)signatureClz);
    jvmti->Deallocate((unsigned char*)genericClz);
}

// 去重处理
/*2021-11-07 13:56:24.218 28501-28501/com.hana.detection I/jvmti: stack is Landroid/app/ApplicationPackageManager; getInstalledApplications,926
2021-11-07 13:56:24.218 28501-28501/com.hana.detection I/jvmti: stack is Lcom/hana/detection/MainActivity; getInstalled,127
2021-11-07 13:56:24.218 28501-28501/com.hana.detection I/jvmti: stack is Lcom/hana/detection/MainActivity; testHello,52
2021-11-07 13:56:24.218 28501-28501/com.hana.detection I/jvmti: stack is Lcom/hana/detection/MainActivity; testHello,53
2021-11-07 13:56:24.218 28501-28501/com.hana.detection I/jvmti: stack is Lcom/hana/detection/MainActivity; onClick,44
2021-11-07 13:56:24.218 28501-28501/com.hana.detection I/jvmti: stack is Lcom/hana/detection/MainActivity; onClick,45
2021-11-07 13:56:24.218 28501-28501/com.hana.detection I/jvmti: stack is Lcom/hana/detection/MainActivity; onClick,46*/

void printMatchMethodStack(jvmtiEnv *jvmti, JNIEnv *env,
                           jthread thr, jmethodID method) {
    char *name;
    char *signature;
    char *generic;

    jclass clazz;

    auto error = (*jvmti).GetMethodName(method, &name, &signature, &generic);
    auto errClass = jvmti->GetMethodDeclaringClass(method, &clazz);

    if (error == JVMTI_ERROR_NONE && errClass == JVMTI_ERROR_NONE) {
        if (strcmp(name, "getInstalledApplications") == 0) {
            ALOGI("检测到敏感方法 %s", name);
            jvmtiFrameInfo frames[5];
            int count;
            (*jvmti).GetStackTrace(thr, 0, 5, frames, &count);

            for (auto & frame : frames) {
                char *stackName;
                char *stackSignature;
                char *stackGeneric;
                (*jvmti).GetMethodName(frame.method, &stackName, &stackSignature,
                                       &stackGeneric);
                jint lineCount = 0;
                jvmtiLineNumberEntry *lineTable = nullptr;
                int lineNumber = -1;

                jvmtiError lineError = (*jvmti).GetLineNumberTable(frame.method, &lineCount, &lineTable);
                if (lineError == JVMTI_ERROR_NONE) {
                    lineNumber = lineTable[0].line_number;
                    for (int i = 0; i < lineCount; i++) {
                        if (frame.location < lineTable[i].start_location) {
                            break;
                        }
                        lineNumber = lineTable[i].line_number;

                        char *signatureClz;
                        char *genericClz;

                        jclass frameClazz;
                        jvmti->GetMethodDeclaringClass(frame.method, &frameClazz);
                        auto errClass = (*jvmti).GetClassSignature(frameClazz, &signatureClz,
                                                                   &genericClz);
                        ALOGI("stack is %1$s %2$s,%3$d", signatureClz,stackName, lineNumber);
                        jvmti->Deallocate((unsigned char*)signatureClz);
                        jvmti->Deallocate((unsigned char*)genericClz);
                    }
                } else if (lineError != JVMTI_ERROR_ABSENT_INFORMATION) {
                    // JVMTI_ERROR_NULL_POINTER https://docs.oracle.com/javase/7/docs/platform/jvmti/jvmti.html#GetLineNumberTable
                    ALOGI("GetLineNumberTable error: %1$s,%2$i", "Cannot get method line table",
                          error);
                }
                //ALOGI("jvmtiFrameInfo is %1$s,%2$lld",stackName,(int64_t)frames[dep].location);

                jvmti->Deallocate((unsigned char*)stackName);
                jvmti->Deallocate((unsigned char*)stackSignature);
                jvmti->Deallocate((unsigned char*)stackGeneric);
            }
        }
    }

    jvmti->Deallocate((unsigned char*)name);
    jvmti->Deallocate((unsigned char*)signature);
    jvmti->Deallocate((unsigned char*)generic);
    // jni->ReleaseStringUTFChars(name,className);
}

extern "C" void JNICALL callbackMethodExit(jvmtiEnv *jvmti,
                                           JNIEnv *jni_env,
                                           jthread thread,
                                           jmethodID method,
                                           jboolean was_popped_by_exception,
                                           jvalue return_value) {
    /*char *name;
    char *signature;
    char *generic;

    (*jvmti).GetMethodName(method, &name, &signature, &generic);*/
}
extern "C" void JNICALL callbackMethodEntry(jvmtiEnv *jvmti, JNIEnv *env,
                                            jthread thr, jmethodID method) {
    jvmti->RawMonitorEnter(gb_lock);
    printInvokeStack(jvmti,env,thr,method);
    //printMatchMethodStack(jvmti, env, thr, method);
    jvmti->RawMonitorExit(gb_lock);
}

extern "C" JNIEXPORT jint JNICALL
Agent_OnLoad(JavaVM *vm, char *options,
             void *reserved) {
    ALOGI("==============Agent_OnLoad====================");
}

extern "C" JNIEXPORT jint JNICALL
Agent_OnAttach(JavaVM *vm, char *options,
               void *reserved) {
    ALOGI("==============Agent_OnAttach====================");
    jvmtiEnv *jvmti = CreateJvmtiEnv(vm);

    jvmtiCapabilities capabilities;
    (void) memset(&capabilities, 0, sizeof(jvmtiCapabilities));
    capabilities.can_generate_method_entry_events = 1;
    capabilities.can_generate_method_exit_events = 1;
    capabilities.can_access_local_variables = 1;
    capabilities.can_get_line_numbers = 1;

    jvmti->CreateRawMonitor("agent data", &gb_lock);
    auto error = (*jvmti).AddCapabilities(&capabilities);

    if (error != JVMTI_ERROR_NONE) {
        ALOGI("ERROR: Unable to AddCapabilities JVMTI");
        return error;
    }

    jvmtiEventCallbacks callbacks;
    callbacks.MethodEntry = &callbackMethodEntry;
    callbacks.MethodExit = &callbackMethodExit;

    error = (*jvmti).SetEventCallbacks(&callbacks, (jint) sizeof(callbacks));
    if (error != JVMTI_ERROR_NONE) {
        ALOGI("ERROR: Unable to SetEventCallbacks JVMTI!");
        return error;
    }

    error = (*jvmti).SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_METHOD_ENTRY, nullptr);
    if (error != JVMTI_ERROR_NONE) {
        ALOGI("ERROR: Unable to SetEventNotificationMode JVMTI!");
        return error;
    }

    error = (*jvmti).SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_METHOD_EXIT, nullptr);
    if (error != JVMTI_ERROR_NONE) {
        ALOGI("ERROR: Unable to SetEventNotificationMode JVMTI!");
        return error;
    }

    return JNI_OK;
}
