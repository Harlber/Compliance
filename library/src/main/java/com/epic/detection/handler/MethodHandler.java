package com.epic.detection.handler;

import android.util.Log;


import com.epic.detection.MethodDetection;

import de.robv.android.xposed.XC_MethodHook;

public class MethodHandler extends XC_MethodHook {
    private static final String TAG = MethodDetection.TAG;

    @Override
    protected void beforeHookedMethod(MethodHookParam param) throws Throwable {
        super.beforeHookedMethod(param);
        Log.i(TAG, "**检测到风险函数调用-> " +param.method.getDeclaringClass().getName()+"#"+ param.method.getName());
        Log.d(TAG, getMethodStack());
    }

    public String getMethodStack() {
        StackTraceElement[] stackTraceElements = Thread.currentThread().getStackTrace();
        boolean skip = true;
        StringBuilder stringBuilder = new StringBuilder();
        stringBuilder.append('\n');
        for (StackTraceElement temp : stackTraceElements) {
            String line = temp.toString();
            boolean isEpic = line.startsWith("me.weishu.epic");

            if (!skip && !isEpic){
                stringBuilder.append(line);
                stringBuilder.append('\n');
            }

            if (isEpic && skip){
                skip = false;
            }
        }

        return stringBuilder.toString();

    }
}
