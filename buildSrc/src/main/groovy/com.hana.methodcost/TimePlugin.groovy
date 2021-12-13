package com.hana.methodcost

import com.android.build.api.transform.*
import com.android.build.gradle.AppExtension
import com.android.build.gradle.internal.pipeline.TransformManager
import org.apache.commons.codec.digest.DigestUtils
import org.apache.commons.io.FileUtils
import org.apache.commons.io.IOUtils
import org.gradle.api.Plugin
import org.gradle.api.Project
import org.objectweb.asm.ClassReader
import org.objectweb.asm.ClassVisitor
import org.objectweb.asm.ClassWriter

import java.util.jar.JarEntry
import java.util.jar.JarFile
import java.util.jar.JarOutputStream
import java.util.zip.ZipEntry

import static org.objectweb.asm.ClassReader.EXPAND_FRAMES

public class TimePlugin extends Transform implements Plugin<Project> {
    def jarEnable = true

    void apply(Project project) {
        project.extensions.create("timecost", TimeCostExtension)

        def android = project.extensions.getByType(AppExtension);
        android.registerTransform(this)

        project.afterEvaluate {
            def extension = project.extensions.findByName("timecost") as TimeCostExtension
            jarEnable = extension.jarEnable
        }
    }


    @Override
    public String getName() {
        return "TimePlugin";
    }

    @Override
    public Set<QualifiedContent.ContentType> getInputTypes() {
        return TransformManager.CONTENT_CLASS;
    }

    @Override
    public Set<QualifiedContent.Scope> getScopes() {
        return TransformManager.SCOPE_FULL_PROJECT;
    }

    @Override
    public boolean isIncremental() {
        return false
    }

    @Override
    void transform(Context context, Collection<TransformInput> inputs, Collection<TransformInput> referencedInputs,
                   TransformOutputProvider outputProvider, boolean isIncremental)
            throws IOException, TransformException, InterruptedException {
        println '//===============TracePlugin visit start===============//'

        def startTime = System.currentTimeMillis()
        if (outputProvider != null)
            outputProvider.deleteAll()
        inputs.each { TransformInput input ->
            input.directoryInputs.each { DirectoryInput directoryInput ->

                if (directoryInput.file.isDirectory()) {
                    //遍历目录
                    directoryInput.file.eachFileRecurse {
                        File file ->
                            def filename = file.name;
                            def name = file.name
                            // 处理源码
                            if (name.endsWith(".class") && !name.startsWith("R\$") &&
                                    !"R.class".equals(name) && !"BuildConfig.class".equals(name)) {
                                println("=============directory name = " + name)
                                ClassReader classReader = new ClassReader(file.bytes)
                                ClassWriter classWriter = new ClassWriter(classReader, ClassWriter.COMPUTE_MAXS)
                                def className = name.split(".class")[0]
                                ClassVisitor cv = new TraceVisitor(className, classWriter)
                                classReader.accept(cv, EXPAND_FRAMES)
                                byte[] code = classWriter.toByteArray()
                                FileOutputStream fos = new FileOutputStream(
                                        file.parentFile.absolutePath + File.separator + name)
                                fos.write(code)
                                fos.close()

                            }
                    }
                }

                def dest = outputProvider.getContentLocation(directoryInput.name,
                        directoryInput.contentTypes, directoryInput.scopes,
                        Format.DIRECTORY)


                FileUtils.copyDirectory(directoryInput.file, dest)
            }

            input.jarInputs.each { JarInput jarInput ->
                if (jarInput.file) {
                    System.out.println("jarInput is direcotry name=" + jarInput.name)
                }
                if (jarEnable) {
                    handleJarInputs(jarInput, outputProvider)
                } else {
                    def jarName = jarInput.name
                    if (jarInput.file) {
                        System.out.println("jarInput is direcotry name=" + jarName)
                    }

                    def md5Name = DigestUtils.md5Hex(jarInput.file.getAbsolutePath())
                    if (jarName.endsWith(".jar")) {
                        jarName = jarName.substring(0, jarName.length() - 4)
                    }

                    def dest = outputProvider.getContentLocation(jarName + md5Name,
                            jarInput.contentTypes, jarInput.scopes, Format.JAR)

                    FileUtils.copyFile(jarInput.file, dest)
                }
            }
        }

        def cost = (System.currentTimeMillis() - startTime) / 1000
        println "plugin cost $cost secs"
        println '//===============TracePlugin visit end===============//'
    }

    static void handleJarInputs(JarInput jarInput, TransformOutputProvider outputProvider) {
        if (jarInput.file.getAbsolutePath().endsWith(".jar")) {
            //重名名输出文件,因为可能同名,会覆盖
            def jarName = jarInput.name
            def md5Name = DigestUtils.md5Hex(jarInput.file.getAbsolutePath())
            if (jarName.endsWith(".jar")) {
                jarName = jarName.substring(0, jarName.length() - 4)
            }
            JarFile jarFile = new JarFile(jarInput.file)
            Enumeration enumeration = jarFile.entries()
            File tmpFile = new File(jarInput.file.getParent() + File.separator + "classes_temp.jar")
            //避免上次的缓存被重复插入
            if (tmpFile.exists()) {
                tmpFile.delete()
            }
            JarOutputStream jarOutputStream = new JarOutputStream(new FileOutputStream(tmpFile))
            //用于保存
            while (enumeration.hasMoreElements()) {
                JarEntry jarEntry = (JarEntry) enumeration.nextElement()
                String entryName = jarEntry.getName()
                ZipEntry zipEntry = new ZipEntry(entryName)
                InputStream inputStream = jarFile.getInputStream(jarEntry)
                //插桩class
                if (entryName.endsWith(".class") && !entryName.startsWith("R\$")
                        && !"R.class".equals(entryName) && !"BuildConfig.class".equals(entryName)
                        && filterJarByName(entryName)) {
                    //jar中的 class文件处理
                    println '----------- deal with "jar" class file <' + entryName + '> -----------'
                    // entryName：<com/amap/api/services/route/DrivePlanStep.class>
                    jarOutputStream.putNextEntry(zipEntry)
                    ClassReader classReader = new ClassReader(IOUtils.toByteArray(inputStream))
                    ClassWriter classWriter = new ClassWriter(classReader, ClassWriter.COMPUTE_MAXS)
                    ClassVisitor cv = new TraceVisitor("", classWriter)
                    classReader.accept(cv, EXPAND_FRAMES)
                    byte[] code = classWriter.toByteArray()
                    jarOutputStream.write(code)
                } else {
                    jarOutputStream.putNextEntry(zipEntry)
                    jarOutputStream.write(IOUtils.toByteArray(inputStream))
                }
                jarOutputStream.closeEntry()
            }
            //结束
            jarOutputStream.close()
            jarFile.close()
            def dest = outputProvider.getContentLocation(jarName + md5Name,
                    jarInput.contentTypes, jarInput.scopes, Format.JAR)
            FileUtils.copyFile(tmpFile, dest)
            tmpFile.delete()
        }
    }

    //need modify to what u want
    // <com/android/widget/TextView$LocalBinder.class>
    // <com/android/widget/TextView.class>
    static boolean filterJarByName(String jarName) {
        return jarName.contains("android")
    }
}