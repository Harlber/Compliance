package com.hana.methodcost

import org.gradle.api.Plugin
import org.gradle.api.Project

public class CustomPlugin implements Plugin<Project> {

    @Override
    void apply(Project project) {
        project.tasks.register("hello", HelloTask){
            dependsOn("build")
            doLast {
                print('   hello from CustomPlugin')
            }
        }
    }
}