
## 备注

记录一些问题和经验。

### Android

1. 在 Android Studio 中将外部模块导入到 Android 项目 

   配置 gradle 文件以使用其他文件夹，而无需将整个模块复制到您的项目中。 

   - 从导入项目配置gradle 
      
      您可以在项目中配置 gradle 文件，并且您的应用程序可以将一个文件夹与您的其他项目一起使用，而无需在项目文件夹中创建子文件夹。 这是支持您附加模块的更好方法，因为如果您向附加库添加一些更改，您的所有项目都将使用此模块进行所有更改。 

      我们需要在“主项目”中打开 settings.gradle 文件，我们需要添加几行： 

      ```gradle
      include ':sharedlib_name'
      project(':sharedlib_name').projectDir = new File(settingsDir, '../../../android/sharedlib_name')
      ```

      之后，我们需要在 app 项目中的 build.gradle 文件中添加一个依赖项，我们需要将下面这一行添加到这个文件中： 

      ```gradle
      implementation project(path: ':sharedlib_name')
      ```

      这些更改后，您将使用外部模块的默认文件夹中的模块。 

2. 在 Android 项目中包名（也是路径名）不要用下划线**_**。

   下划线'_'会导致，做c++的jni接口时，下划线后面加'1'。比如`sharedlib_name`会在jni中变成`sharedlib_1name`。

3. Android需要安装NDK和对应的cmake，可以根据Android Studio的提示来安装。

### macOS

1. xcconfig

   通过Xcode添加一个新文件，文件类型选**Configuration Settings File**. [参考](https://nshipster.com/xcconfig/).  此外，创建之后，还需要在工程配置里配置上。

2. **MACOSX_FRAMEWORK_IDENTIFIER**不能用下划线`_`，可以用`-`。
   