package com.epic.detection.handler;


import android.util.Log;

import com.epic.detection.MethodDetection;


/**
 * @file ContentResolverHandler
 */
public class SettingsResolverHandler extends MethodHandler {
    private static final String TAG = MethodDetection.TAG;

    @Override
    protected void beforeHookedMethod(MethodHookParam param) throws Throwable {
        Log.i(TAG, "**检测到风险系统设置查询: "+param.args[1].toString());
        Log.d(TAG, getMethodStack());
        super.beforeHookedMethod(param);
    }
}
