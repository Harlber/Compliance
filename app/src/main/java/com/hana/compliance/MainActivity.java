package com.hana.compliance;

import androidx.appcompat.app.AppCompatActivity;

import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.util.Log;
import android.view.View;


import com.epic.detection.MethodDetection;

import java.util.List;

public class MainActivity extends AppCompatActivity {
    private static final String TAG = "MainActivity";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        // JvmtiLib.initJvmti(this);  //jvmti 获取方法调用
        MethodDetection.start(); // epic hook 已兼容android 5-12

        findViewById(R.id.tv_main_getinstalledpackage).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                getInstalled();
            }
        });
    }

    public void getAllAppNames() {
        PackageManager pm = getPackageManager();
        ////获取到所有安装了的应用程序的信息，包括那些卸载了的，但没有清除数据的应用程序
        List<PackageInfo> list2 = pm.getInstalledPackages(PackageManager.GET_UNINSTALLED_PACKAGES);

        int j = 0;

        for (PackageInfo packageInfo : list2) {
            //得到手机上已经安装的应用的名字,即在AndriodMainfest.xml中的app_name。
            String appName = packageInfo.applicationInfo.loadLabel(getPackageManager()).toString();
            //得到手机上已经安装的应用的图标,即在AndriodMainfest.xml中的icon。
            Drawable drawable = packageInfo.applicationInfo.loadIcon(getPackageManager());
            //得到应用所在包的名字,即在AndriodMainfest.xml中的package的值。
            String packageName = packageInfo.packageName;
            Log.i(TAG, "应用的名字:" + appName);
            Log.i(TAG, "应用的包名字:" + packageName);

            j++;
        }
        Log.e(TAG, "应用的总个数:" + j);
    }

    private void getInstalled() {
        List<ApplicationInfo> mApp = getPackageManager().getInstalledApplications(0);
        if (mApp != null) {
            Log.i(TAG, "getInstalledApplications not null");
        }
    }
}