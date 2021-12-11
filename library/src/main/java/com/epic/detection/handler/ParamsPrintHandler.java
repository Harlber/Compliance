package com.epic.detection.handler;


import android.util.Log;

import com.epic.detection.MethodDetection;


/**
 * @file ContentResolverHandler
 */
public class ParamsPrintHandler extends MethodHandler {
    private static final String TAG = MethodDetection.TAG;

    @Override
    protected void beforeHookedMethod(MethodHookParam param) throws Throwable {
        StringBuilder stringBuilder = new StringBuilder();
        for (Object arg : param.args) {
            stringBuilder.append("[");
            stringBuilder.append(arg.toString());
            stringBuilder.append("],");
        }
        Log.i(TAG, "**参数打印: "+stringBuilder.toString());
        super.beforeHookedMethod(param);
    }
}
