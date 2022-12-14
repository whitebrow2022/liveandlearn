# liveandlearn

## xcode笔记
1. xcode调试Qt的QString
   - 断点后，在lldb使用命令`print qstr.toStdString()`.

## git笔记

1. windows端clone仓库时指定lf转crlf:
   ```
   git clone https://github.com/group/repo.git --config core.autocrlf=true 
   ```
2. 本地合并多个commit到一个commit. 在这个例子中，我们将压缩最后 3 次提交。 并且强制推送到远端。
   ```bash
   git reset --soft HEAD~3 && git commit
   git push --force origin HEAD
   ```

## python笔记

1. mac端安装python2.7
   ```
   brew install pyenv
   pyenv install 2.7.18
   # Set the python version.
   pyenv global 2.7.18
   # Export PATH if necessary.
   export PATH="$(pyenv root)/shims:${PATH}"
   # Add if necessary.:
   echo 'PATH=$(pyenv root)/shims:$PATH' >> ~/.zshrc
   echo 'PATH=$(pyenv root)/shims:$PATH' >> ~/.bash_profile
   ```
