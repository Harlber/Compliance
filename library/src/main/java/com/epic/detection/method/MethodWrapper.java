package com.epic.detection.method;


import com.epic.detection.MethodDetection;
import com.epic.detection.handler.MethodHandler;
import com.epic.detection.handler.ParamsPrintHandler;

public class MethodWrapper {
    private Class<?> targetClass;
    private String targetMethod;
    private Class<?>[] params;
    private MethodHandler methodHandler;

    public static MethodWrapper newPrint(Class<?> targetClass, String targetMethod, Class<?>... params){
        MethodWrapper temp = new MethodWrapper(targetClass, targetMethod, params);
        temp.setMethodHandler(new ParamsPrintHandler());
        return temp;
    }

    public MethodWrapper(Class<?> targetClass, String targetMethod, Class<?>... params) {
        this.targetClass = targetClass;
        this.targetMethod = targetMethod;
        this.params = params;
    }

    public void setMethodHandler(MethodHandler methodHandler) {
        this.methodHandler = methodHandler;
    }

    public Class<?> getTargetClass() {
        return targetClass;
    }

    public String getTargetMethod() {
        return targetMethod;
    }

    public Class<?>[] getParams() {
        return params;
    }

    public Object[] getParamsWithDefaultHandler(){
        Object[] temp = new Object[params.length+1];
        System.arraycopy(params, 0, temp, 0, params.length);
        if (methodHandler == null){
            temp[params.length] = MethodDetection.defaultMethodHandler;
        }else {
            temp[params.length] = methodHandler;
        }
        return temp;
    }
}
