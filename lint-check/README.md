 ## lint检测合规api

 1.检测源码中的api调用
 2.检测三方jar中的api调用
 3.检测三方aar中的api调用

 lint官方文档 http://googlesamples.github.io/android-custom-lint-rules/api-guide.html
 lint官方demo https://github.com/googlesamples/android-custom-lint-rules
 lint使用官方指南 https://developer.android.com/studio/write/lint

 [关于lint-api的版本](http://googlesamples.github.io/android-custom-lint-rules/api-guide.html)：
 The lint version of the libraries (specified in this project as the lintVersion variable in build.gradle) should be the same version that is used by the Gradle plugin.
 If the Gradle plugin version is X.Y.Z, then the Lint library version is X+23.Y.Z.
 For example, for AGP 7.0.0-alpha08, the lint API versions are 30.0.0-alpha08.

 [android-gradle-plugin版本与gradle版本对应关系](https://developer.android.com/studio/releases/gradle-plugin)
 [android-gradle-plugin版本](https://mvnrepository.com/artifact/com.android.tools.build/gradle?repo=google)
 [lint-api文档](https://www.javadoc.io/static/com.android.tools.lint/lint-api/25.5.0-alpha-preview-02/overview-summary.html)
 [lint api版本](https://mvnrepository.com/artifact/com.android.tools.lint/lint-api?repo=google)


lint不能扫aar的bug https://issuetracker.google.com/138666165 ｜ https://groups.google.com/g/lint-dev/c/hWnrH7RI7JM/m/Be74BXdiCQAJ
经过确认和AGP版本有关
3.4.0  no
3.6.4  no
4.0.1 no
4.0.2 no
4.1.0 yes

lint-api的版本怎么确认呢，简单的，运行./gradlew :app:depend 看下 `com.android.tools.lint:lint-gradle-api`的版本号是多少

 lint源码，如何debug