package com.epic.detection;

import android.util.Log;

import androidx.annotation.Keep;

import com.epic.detection.handler.MethodHandler;
import com.epic.detection.method.ClassMethodGroup;
import com.epic.detection.method.HookMethodList;
import com.epic.detection.method.MethodWrapper;
import com.epic.detection.method.NormalMethodList;
import com.epic.detection.method.StrictMethodGroup;

import java.lang.reflect.Method;


import de.robv.android.xposed.DexposedBridge;

/**
 * https://github.com/tiann/epic/blob/master/README_cn.md
 * xposed https://api.xposed.info/reference/de/robv/android/xposed/XposedHelpers.html
 * https://github.com/JnuSimba/AndroidSecNotes/blob/master/Android%20%E8%B0%83%E8%AF%95%E5%B7%A5%E5%85%B7/Android%20Hook%20%E6%A1%86%E6%9E%B6%EF%BC%88XPosed%E7%AF%87%EF%BC%89.md
 */
@Keep
public class MethodDetection {
    public static final String TAG = "MethodDetection";
    /**
     * 严格模式
     */
    private static final boolean SYSTEM_STRICT_MODE = false;

    public static final MethodHandler defaultMethodHandler = new MethodHandler();


    public static void start() {
        loadHookMethod();
    }

    private static void loadHookMethod() {
        //合规收集处理方法
        HookMethodList methodList = new NormalMethodList();
        for (MethodWrapper methodWrapper : methodList.getMethodList()) {
            registerMethod(methodWrapper);
        }
        for (ClassMethodGroup classMethodGroup : methodList.getAbsMethodList()) {
            registerClass(classMethodGroup);
        }

        /*if (SYSTEM_STRICT_MODE) {
            loadStrictMethod();
        }*/
    }

    private static void loadStrictMethod() {
        StrictMethodGroup strictMethodGroup = new StrictMethodGroup();
        for (ClassMethodGroup classMethodGroup : strictMethodGroup.getClassGroupList()) {
            registerClass(classMethodGroup);
        }
    }

    /**
     * 通过类名，内部查找方法进行hook
     */
    private static void registerMethod(MethodWrapper methodWrapper) {
        try {
            DexposedBridge.findAndHookMethod(methodWrapper.getTargetClass(), methodWrapper.getTargetMethod(), new MethodHandler());
        } catch (NoSuchMethodError ignore) {
            //Log.wtf(TAG, "registeredMethod: NoSuchMethodError->" + error.getMessage());
        }
    }


    /**
     * 通过类名，反射查找，适用于 Override 方法
     */
    private static void registerClass(ClassMethodGroup classMethodGroup) {
        try {
            Class<?> clazz = Class.forName(classMethodGroup.getTargetClassName());
            Method[] declareMethods = clazz.getDeclaredMethods();

            for (Method method : declareMethods) {
                if (classMethodGroup.getMethodGroup().contains(method.getName())) {
                    DexposedBridge.hookMethod(method, defaultMethodHandler);
                }
            }

        } catch (Exception e) {
            Log.wtf(TAG, "registerClass Error-> " + e.getMessage());
        }
    }

}
