package com.hana.lintcheck.detector

import com.android.tools.lint.detector.api.*
import org.objectweb.asm.Opcodes
import org.objectweb.asm.tree.AbstractInsnNode
import org.objectweb.asm.tree.ClassNode
import org.objectweb.asm.tree.MethodInsnNode
import org.objectweb.asm.tree.MethodNode


class ComplianceDetector : Detector(), ClassScanner {

    companion object {
        val ISSUE = Issue.create(
            "ComplianceDetector",//问题 Id
            "不要调用相关合规api",//问题的简单描述，会被 report 接口传入的描述覆盖
            "调用危险合规api可能会导致下架",
            Category.SECURITY,
            5,
            Severity.ERROR,
            Implementation(
                ComplianceDetector::class.java,
                Scope.ALL_CLASSES_AND_LIBRARIES
            )
        )
    }

    override fun getApplicableAsmNodeTypes(): IntArray? {
        return intArrayOf(AbstractInsnNode.METHOD_INSN)
    }

    override fun checkInstruction(context: ClassContext, classNode: ClassNode, method: MethodNode, instruction: AbstractInsnNode) {
        if (instruction.opcode != Opcodes.INVOKEVIRTUAL) {
            return
        }
        val callerMethodSig = classNode.name + "." + method.name + method.desc
        val methodInsn = instruction as MethodInsnNode
        if (methodInsn.name == "getDeviceId"&&methodInsn.owner=="android/telephony/TelephonyManager") {
            val message = "SDK 中 $callerMethodSig 调用了 " +
                    "${methodInsn.owner.substringAfterLast('/')}.${methodInsn.name} 的方法，需要注意！"
            context.report(ISSUE, method, methodInsn, context.getLocation(methodInsn), message)
        }
    }
}