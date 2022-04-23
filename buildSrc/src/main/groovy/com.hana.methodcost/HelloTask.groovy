package com.hana.methodcost

import org.gradle.api.DefaultTask
import org.gradle.api.tasks.TaskAction

public class HelloTask extends DefaultTask{

    @TaskAction
    taskAction(){
        print("HelloTask taskAction")
    }
}