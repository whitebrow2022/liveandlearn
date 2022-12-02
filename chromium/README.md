# chromium代码验证

## 编译
1. 克隆chromium代码
   - win端：[chromium构建](https://chromium.googlesource.com/chromium/src/+/main/docs/windows_build_instructions.md)
2. 把下面这段代码写到根目录下**chromium/src/BUILD.gn**:
   ```gn
   group("liveandlearn") {
     public_deps = [
       "//liveandlearn/chromium:chromium_test",
     ]
   } 
   ```
