 ## lint检测合规api

思路：
 - 检测源码中的api调用
 - 检测三方jar中的api调用
 - 检测三方aar中的api调用

有用的参考链接：

|  name   | link  |
|  ----  | ----  |
| lint官方文档  | http://googlesamples.github.io/android-custom-lint-rules/api-guide.html |
| lint官方demo  | https://github.com/googlesamples/android-custom-lint-rules |
| lint使用官方指南  | https://developer.android.com/studio/write/lint |


 [关于lint-api的版本](http://googlesamples.github.io/android-custom-lint-rules/api-guide.html)：
 > The lint version of the libraries (specified in this project as the lintVersion variable in build.gradle) should be the same version that is used by the Gradle plugin.
 If the Gradle plugin version is X.Y.Z, then the Lint library version is X+23.Y.Z.
 For example, for AGP 7.0.0-alpha08, the lint API versions are 30.0.0-alpha08.
## lint-api的版本确认
简单的，运行./gradlew :app:depend 看下 `com.android.tools.lint:lint-gradle-api`的版本号是多少

## AGP,gradle,lint-api 版本对应关系
 - [android-gradle-plugin版本与gradle版本对应关系](https://developer.android.com/studio/releases/gradle-plugin)
 - [android-gradle-plugin版本](https://mvnrepository.com/artifact/com.android.tools.build/gradle?repo=google)
 - [lint-api文档](https://www.javadoc.io/static/com.android.tools.lint/lint-api/25.5.0-alpha-preview-02/overview-summary.html)
 - [lint api版本](https://mvnrepository.com/artifact/com.android.tools.lint/lint-api?repo=google)


## 关于lint不能扫aar的bug

 - https://issuetracker.google.com/138666165 
 - https://groups.google.com/g/lint-dev/c/hWnrH7RI7JM/m/Be74BXdiCQAJ

经确认和AGP版本有关
+ `3.4.0` 不支持
+ `3.6.4` 不支持
+ `4.0.1` 不支持
+ `4.0.2` 不支持
+ `4.1.0` 支持
    


## lint源码debug

<img src="img/img_add_remote.png" width="1000px" />
<img src="img/img_add_remote_set.png" width="1000px" />

`Terminal` 中输入
 > ./gradlew lintDebug -Dorg.gradle.daemon=false -Dorg.gradle.debug=true
