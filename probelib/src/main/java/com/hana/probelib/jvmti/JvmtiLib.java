package com.hana.probelib.jvmti;

import android.content.Context;
import android.os.Build;
import android.os.Debug;
import android.util.Log;

import androidx.annotation.Keep;

import java.io.File;
import java.lang.reflect.Method;
import java.nio.file.Files;
import java.nio.file.Paths;

@Keep
public class JvmtiLib {

    private static final String LIB_NAME = "probelib";
    // Used to load the 'probelib' library on application startup.
/*    static {
// 手动调用load
        System.loadLibrary("probelib");
    }*/

    public static void initJvmti(Context context) {
        try {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                String packageCodePath = context.getPackageCodePath();
                ClassLoader classLoader = context.getClassLoader();
                Method findLibrary = ClassLoader.class.getDeclaredMethod("findLibrary", String.class);
                String jvmtiAgentLibPath = (String) findLibrary.invoke(classLoader, LIB_NAME);
                //copy lib to /data/data/com.dodola.jvmti/files/jvmti
                Log.d("jvmtiagentlibpath", "jvmtiagentlibpath " + jvmtiAgentLibPath);
                File filesDir = context.getFilesDir();
                File jvmtiLibDir = new File(filesDir, "jvmti");
                if (!jvmtiLibDir.exists()) {
                    jvmtiLibDir.mkdirs();

                }
                File agentLibSo = new File(jvmtiLibDir, "agent.so");
                if (agentLibSo.exists()) {
                    agentLibSo.delete();
                }
                Files.copy(Paths.get(new File(jvmtiAgentLibPath).getAbsolutePath()), Paths.get((agentLibSo).getAbsolutePath()));

                Log.d("Jvmti", agentLibSo.getAbsolutePath() + "," + context.getPackageCodePath());

                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
                    Debug.attachJvmtiAgent(agentLibSo.getAbsolutePath(), null, classLoader);
                } else {
                    try {
                        Class vmDebugClazz = Class.forName("dalvik.system.VMDebug");
                        Method attachAgentMethod = vmDebugClazz.getMethod("attachAgent", String.class);
                        attachAgentMethod.setAccessible(true);
                        attachAgentMethod.invoke(null, agentLibSo.getAbsolutePath());
                    } catch (Exception ex) {
                        ex.printStackTrace();
                    }
                }
                System.loadLibrary(LIB_NAME);
            }
        } catch (Exception ex) {
            ex.printStackTrace();
        }
    }
}