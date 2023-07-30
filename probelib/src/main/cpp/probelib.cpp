#include <jni.h>
#include <string>
#include <android/log.h>

#include "jvmti.h"

#include <sys/mman.h>
#include <unistd.h>
#include <unwind.h>
#include <dlfcn.h>
#include <stdarg.h>

#define LOG_TAG "jvmti"

#define ALOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#define ALOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define ALOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#define LOG(fmt, ...) __android_log_print(ANDROID_LOG_ERROR, "native hook", fmt, ##__VA_ARGS__)

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

struct GlobalData{
    JavaVM  *jvm;
    jvmtiEnv  *jvmtiEnv1;
};
GlobalData globalData;


// start native hook

void getPid(JNIEnv *env){
    // https://developer.android.com/training/articles/perf-jni?hl=zh-cn#jclass-jmethodid-and-jfieldid
    const char *classStr = "android/os/Process";
    jclass localClass = env->FindClass(classStr);
    jclass globalClass = reinterpret_cast<jclass>(env->NewGlobalRef(localClass));

    jmethodID proc_id = env->GetStaticMethodID(globalClass, "myPid", "()I");
    jint prcId = env->CallStaticIntMethod(globalClass, proc_id);
    LOG("hook current pid: %d",  prcId);
    env->DeleteGlobalRef(globalClass);
}

char* getMethodName(jmethodID method){
    char* method_name;
    jvmtiError error = globalData.jvmtiEnv1->GetMethodName(method, &method_name, NULL, NULL);
    if (error == JVMTI_ERROR_NONE){
        LOG("hook getMethodName: %s",  method_name);
    }

    return method_name;
    //globalData.jvmtiEnv1->Deallocate((unsigned char*)method_name);
}

/**
 * JNI/C++: Get Class Name
 * @param env [in] JNI context
 * @param myCls [in] Class object, which the name is asked of
 * @param fullpath [in] true for full class path, else name without package context
 * @return Name of class myCls, encoding UTF-8
 */
std::string getClassName(JNIEnv* env, jclass myCls, bool fullpath)
{
    jclass ccls = env->FindClass("java/lang/Class");
    jmethodID mid_getName = env->GetMethodID(ccls, "getName", "()Ljava/lang/String;");
    jstring strObj = (jstring)env->CallObjectMethod(myCls, mid_getName);
    const char* localName = env->GetStringUTFChars(strObj, 0);
    std::string res = localName;
    env->ReleaseStringUTFChars(strObj, localName);
    if (!fullpath)
    {
        std::size_t pos = res.find_last_of('.');
        if (pos!=std::string::npos)
        {
            res = res.substr(pos+1);
        }
    }
    return res;
}

struct BacktraceState
{
    intptr_t* current;
    intptr_t* end;
};


static _Unwind_Reason_Code unwindCallback(struct _Unwind_Context* context, void* arg)
{
    BacktraceState* state = static_cast<BacktraceState*>(arg);
    intptr_t ip = (intptr_t)_Unwind_GetIP(context);
    if (ip) {
        if (state->current == state->end) {
            return _URC_END_OF_STACK;
        } else {
            state->current[0] = ip;
            state->current++;
        }
    }
    return _URC_NO_REASON;


}

size_t captureBacktrace(intptr_t* buffer, size_t maxStackDeep)
{
    BacktraceState state = {buffer, buffer + maxStackDeep};
    _Unwind_Backtrace(unwindCallback, &state);
    return state.current - buffer;
}

void dumpBacktraceIndex(char *out, intptr_t* buffer, size_t count) {
    for (size_t idx = 0; idx < count; ++idx) {
        intptr_t addr = buffer[idx];
        const char *symbol = "      ";
        const char *dlfile = "      ";

        Dl_info info;
        if (dladdr((void *) addr, &info)) {
            if (info.dli_sname) {
                symbol = info.dli_sname;
            }
            if (info.dli_fname) {
                dlfile = info.dli_fname;
            }
        } else {
            strcat(out, "#                               \n");
            continue;
        }
        char temp[50];
        memset(temp, 0, sizeof(temp));
        sprintf(temp, "%zu", idx);
        strcat(out, "#");
        strcat(out, temp);
        strcat(out, ": ");
        memset(temp, 0, sizeof(temp));
        sprintf(temp, "%zu", addr);
        strcat(out, temp);
        strcat(out, "  ");
        strcat(out, symbol);
        strcat(out, "      ");
        strcat(out, dlfile);
        strcat(out, "\n");
    }
}

// 本地变量保留原始指针
// jstring     (*NewStringUTF)(JNIEnv*, const char*);
jstring (*originNewString)(JNIEnv *env,  const char * obj);
jstring NewStringUTFOverride(JNIEnv *env,  const char * obj) {
    LOG("hook NewStringUTF: %s",  obj);
    // getPid(env);
    const size_t maxStackDeep = 20;
    intptr_t stackBuf[maxStackDeep];
    char outBuf[2048];
    memset(outBuf,0,sizeof(outBuf));
    dumpBacktraceIndex(outBuf, stackBuf, captureBacktrace(stackBuf, maxStackDeep));
    LOG(" %s\n", outBuf);

    return originNewString(env, obj);
}

// jmethodID   (*GetStaticMethodID)(JNIEnv*, jclass, const char*, const char*);
jmethodID (*originGetStaticMethodID)(JNIEnv *env, jclass javaclass, const char* name, const char* sig);
jmethodID GetStaticMethodIDOverride(JNIEnv *env, jclass javaclass, const char* name, const char* sig) {
    std::string class_name = getClassName(env, javaclass, true);
    LOG("hook GetStaticMethodID: class:%s  method:%s sign:%s",  class_name.c_str(), name, sig);
    // getPid(env);
    const size_t maxStackDeep = 20;
    intptr_t stackBuf[maxStackDeep];
    char outBuf[2048];
    memset(outBuf,0,sizeof(outBuf));
    dumpBacktraceIndex(outBuf, stackBuf, captureBacktrace(stackBuf, maxStackDeep));
    LOG(" %s\n", outBuf);

    return originGetStaticMethodID(env, javaclass, name, sig);
}

// jint        (*CallStaticIntMethod)(JNIEnv*, jclass, jmethodID, ...);
jint (*originCallStaticIntMethod)(JNIEnv *env,  jclass obj, jmethodID methodId, ...);
jint CallStaticIntMethodOverride(JNIEnv *env,  jclass javaclass, jmethodID methodId, ...) {
    std::string class_name = getClassName(env, javaclass, true);
    LOG("hook CallStaticIntMethod: class: %s",  class_name.c_str());
    const size_t maxStackDeep = 20;
    intptr_t stackBuf[maxStackDeep];
    char outBuf[2048];
    memset(outBuf,0,sizeof(outBuf));
    dumpBacktraceIndex(outBuf, stackBuf, captureBacktrace(stackBuf, maxStackDeep));
    LOG(" %s\n", outBuf);

    va_list args;
    va_start(args, methodId);
    jint result = originCallStaticIntMethod(env, javaclass, methodId, args);
    va_end(args);
    return result;
}

// hook CallStaticIntMethodV: class: android.os.Process, method: myPid
// #0: 486994432656  _Z16captureBacktracePlm      /data/app/~~xCK1gY9tEOG5PvvKCDdk7Q==/com.hana.compliance-6wZ7s5yMUm-09vO7AoFtOg==/lib/arm64/libprobelib.so
// #1: 486994435992  _Z28CallStaticIntMethodVOverrideP7_JNIEnvP7_jclassP10_jmethodIDSt9__va_list      /data/app/~~xCK1gY9tEOG5PvvKCDdk7Q==/com.hana.compliance-6wZ7s5yMUm-09vO7AoFtOg==/lib/arm64/libprobelib.so
// #2: 486994430940  _ZN7_JNIEnv19CallStaticIntMethodEP7_jclassP10_jmethodIDz      /data/app/~~xCK1gY9tEOG5PvvKCDdk7Q==/com.hana.compliance-6wZ7s5yMUm-09vO7AoFtOg==/lib/arm64/libprobelib.so
// #3: 486994430508  _Z6getPidP7_JNIEnv      /data/app/~~xCK1gY9tEOG5PvvKCDdk7Q==/com.hana.compliance-6wZ7s5yMUm-09vO7AoFtOg==/lib/arm64/libprobelib.so
// #4: 486994437280  JNI_OnLoad      /data/app/~~xCK1gY9tEOG5PvvKCDdk7Q==/com.hana.compliance-6wZ7s5yMUm-09vO7AoFtOg==/lib/arm64/libprobelib.so
// #5: 489005059776  _ZN3art9JavaVMExt17LoadNativeLibraryEP7_JNIEnvRKNSt3__112basic_stringIcNS3_11char_traitsIcEENS3_9allocatorIcEEEEP8_jobjectP7_jclassPS9_      /apex/com.android.art/lib6


// jint        (*CallStaticIntMethodV)(JNIEnv*, jclass, jmethodID, va_list);
jint (*originCallStaticIntMethodV)(JNIEnv *env,  jclass obj, jmethodID methodId, va_list vaList);
jint CallStaticIntMethodVOverride(JNIEnv *env,  jclass javaclass, jmethodID methodId, va_list vaList) {
    std::string class_name = getClassName(env, javaclass, true);
    char* method_name = getMethodName(methodId);
    LOG("hook CallStaticIntMethodV: class: %s, method: %s",  class_name.c_str(), method_name);
    globalData.jvmtiEnv1->Deallocate((unsigned char*)method_name);

    const size_t maxStackDeep = 20;
    intptr_t stackBuf[maxStackDeep];
    char outBuf[2048];
    memset(outBuf,0,sizeof(outBuf));
    dumpBacktraceIndex(outBuf, stackBuf, captureBacktrace(stackBuf, maxStackDeep));
    LOG(" %s\n", outBuf);

    return originCallStaticIntMethodV(env, javaclass, methodId, vaList);

}

int accessWrite(unsigned long address) {
    int page_size = getpagesize();
    // 获取 page size 整数倍的起始地址
    address -= (unsigned long)address % page_size;
    if(mprotect((void*)address, page_size, PROT_READ | PROT_WRITE) == -1) {
        LOG("mprotect failed!");
        return -1;
    }
    __builtin___clear_cache((char *)address, (char *)(address + page_size));
    return 0;
}

void hook(JNIEnv *env) {
    auto *functionTable = env->functions;
    void** funcPointerAddress = (void **)&(functionTable->NewStringUTF);
    accessWrite(reinterpret_cast<unsigned long>(funcPointerAddress));
    originNewString = functionTable->NewStringUTF;
    void** newFuncPointerAddress = reinterpret_cast<void **>(&NewStringUTFOverride);
    *funcPointerAddress = newFuncPointerAddress;
}

void hookGetStaticMethodID(JNIEnv *env) {
    auto *functionTable = env->functions;
    // 指向函数指针的变量的地址
    void** funcPointerAddress = (void **)&(functionTable->GetStaticMethodID);
    accessWrite(reinterpret_cast<unsigned long>(funcPointerAddress));
    // 存原始指针
    originGetStaticMethodID = functionTable->GetStaticMethodID;
    // 获取新的函数指针的变量的地址
    void** newFuncPointerAddress = reinterpret_cast<void **>(&GetStaticMethodIDOverride);
    // 替换地址
    *funcPointerAddress = newFuncPointerAddress;
}

void hookCallStaticIntMethod(JNIEnv *env) {
    auto *functionTable = env->functions;
    // 指向函数指针的变量的地址
    void** funcPointerAddress = (void **)&(functionTable->CallStaticIntMethod);
    accessWrite(reinterpret_cast<unsigned long>(funcPointerAddress));
    // 存原始指针
    originCallStaticIntMethod = functionTable->CallStaticIntMethod;
    // 获取新的函数指针的变量的地址
    void** newFuncPointerAddress = reinterpret_cast<void **>(&CallStaticIntMethodOverride);
    // 替换地址
    *funcPointerAddress = newFuncPointerAddress;
}

void hookCallStaticIntMethodV(JNIEnv *env) {
    auto *functionTable = env->functions;
    // 指向函数指针的变量的地址
    void** funcPointerAddress = (void **)&(functionTable->CallStaticIntMethodV);
    accessWrite(reinterpret_cast<unsigned long>(funcPointerAddress));
    // 存原始指针
    originCallStaticIntMethodV = functionTable->CallStaticIntMethodV;
    // 获取新的函数指针的变量的地址
    void** newFuncPointerAddress = reinterpret_cast<void **>(&CallStaticIntMethodVOverride);
    // 替换地址
    *funcPointerAddress = newFuncPointerAddress;
}

// end native hook


jvmtiEnv *CreateJvmtiEnv(JavaVM *vm) {
    jvmtiEnv *jvmti_env;
    jint result = vm->GetEnv((void **) &jvmti_env, JVMTI_VERSION_1_2);

    globalData.jvm = vm;
    vm->GetEnv((void **) &globalData.jvmtiEnv1, JVMTI_VERSION_1_2);

    if (result != JNI_OK) {
        return nullptr;
    }

    return jvmti_env;
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }
    ALOGI("==============JNI_OnLoad====================");
    //hook(env);
    //hookGetStaticMethodID(env);
    //hookCallStaticIntMethod(env);
    jvmtiEnv *jvmti = CreateJvmtiEnv(vm);
    hookCallStaticIntMethodV(env);
    getPid(env);
    return JNI_VERSION_1_6;
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
    //printInvokeStack(jvmti,env,thr,method);
    printMatchMethodStack(jvmti, env, thr, method);
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
