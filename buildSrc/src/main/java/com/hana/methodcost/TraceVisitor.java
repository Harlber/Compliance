package com.hana.methodcost;

import org.objectweb.asm.AnnotationVisitor;
import org.objectweb.asm.ClassVisitor;
import org.objectweb.asm.MethodVisitor;
import org.objectweb.asm.Opcodes;
import org.objectweb.asm.commons.AdviceAdapter;

public class TraceVisitor extends ClassVisitor {

    private String className;

    private String superName;

    private String[] interfaces;

    public TraceVisitor(String className, ClassVisitor classVisitor) {
        super(Opcodes.ASM5, classVisitor);
    }

    @Override
    public MethodVisitor visitMethod(final int access, final String name, final String desc, final String signature,
                                     String[] exceptions) {
        MethodVisitor methodVisitor = cv.visitMethod(access, name, desc, signature, exceptions);
        methodVisitor = new AdviceAdapter(Opcodes.ASM5, methodVisitor, access, name, desc) {

            @Override
            public void visitCode() {
                super.visitCode();
            }

            @Override
            public AnnotationVisitor visitAnnotation(String desc, boolean visible) {
                return super.visitAnnotation(desc, visible);
            }

            @Override
            public void visitFieldInsn(int opcode, String owner, String name, String desc) {
                super.visitFieldInsn(opcode, owner, name, desc);
            }

            @Override
            protected void onMethodEnter() {
                if (className.contains("TimeCost")) {
                    return;
                }
                //System.out.println("onMethodEnter------------ name="+name+", classname="+className);

                mv.visitLdcInsn(className + "_" + name);
                mv.visitMethodInsn(INVOKESTATIC, "com/hana/methodcost/TimeCost", "setStartTime", "(Ljava/lang/String;)V", false);

            }

            @Override
            protected void onMethodExit(int i) {
                super.onMethodExit(i);
                if (className.contains("TimeCost")) {
                    return;
                }

                mv.visitLdcInsn(className + "_" + name);
                mv.visitMethodInsn(INVOKESTATIC, "com/hana/methodcost/TimeCost", "setEndTime", "(Ljava/lang/String;)V", false);

//                mv.visitLdcInsn(className);
//                mv.visitVarInsn(ASTORE, 0);
//                mv.visitLdcInsn(name);
//                mv.visitVarInsn(ASTORE, 1);
//                mv.visitVarInsn(ALOAD, 0);
//                mv.visitVarInsn(ALOAD, 1);
//                mv.visitMethodInsn(INVOKESTATIC, "com/dany/util/TimeCost", "setEndTime", "(Ljava/lang/String;Ljava/lang/String;)V", false);
            }
        };
        return methodVisitor;


    }

    @Override
    public void visit(int version, int access, String name, String signature, String superName, String[] interfaces) {
        super.visit(version, access, name, signature, superName, interfaces);
        this.className = name;
        this.superName = superName;
        this.interfaces = interfaces;
    }
}