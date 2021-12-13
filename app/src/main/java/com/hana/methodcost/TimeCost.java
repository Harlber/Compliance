package com.hana.methodcost;

import java.util.HashMap;
import java.util.Map;

/**
 * TimePlugin日志写入
 * */
public class TimeCost {
    public static Map<String, Long> sStartTime = new HashMap<>();
    private static boolean isEnable = true;

    public static void setStartTime(String name) {
        if(isEnable) {
            String methodName = name.substring(name.lastIndexOf("_")+1);
            String className = name.substring(name.lastIndexOf("/")+1,name.lastIndexOf("_"));
            System.out.println("costtime start:  class: " +className +"  :  ["+methodName+"]");
        }
        sStartTime.put(name, System.currentTimeMillis());
    }

    public static void setEndTime(String name) {
        long start = sStartTime.get(name);
        long end = System.currentTimeMillis();
        try {
            if (isEnable && Long.valueOf(end - start) > 1.0){
                String methodName = name.substring(name.lastIndexOf("_")+1);
                String className = name.substring(name.lastIndexOf("/")+1,name.lastIndexOf("_"));
                System.out.println("costtime end:  class: " +className +"  :  ["+methodName+"] "+ " cost " + Long.valueOf(end - start) + " ms"+", thread="+Thread.currentThread().getId());
            }
        }catch (Exception e){
            System.out.println("costtime end:  name: " +name+ " cost " + Long.valueOf(end - start) + " ms");
        }

    }
}
