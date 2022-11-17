# qtapp 工程模板


```javascript
// 测试独立进程转码
{
  "input": "D:/osknief/imsdk/example.mov",
  "output": "C:/Users/xiufeng.liu/Downloads/mp4s/tmppppp.mp4"
}
{
  "input": "E:/osknief/liveandlearn/all/opencv_wrapper/resources/example.mov",
  "output": "C:/Users/knief/Downloads/mp4s/tmppppp.mp4"
}
```

## Mac: cmake打包dmg
- 如果打包的程序不能运行，用下面这种方法来查看原因：
   - `export QT_DEBUG_PLUGINS=1`
   - 运行app: `xxx_app.app/Contents/MacOS/xxx_app`
   - [参考](https://forum.qt.io/topic/133532/how-to-solve-could-not-load-the-qt-platform-plugin-cocoa-in-even-though-it-was-found/2)
- 检查签名：
   - `codesign -dv --verbose=4 /Path/To/Application.app`