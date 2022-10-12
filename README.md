# liveandlearn

## git笔记

1. windows端clone仓库时指定lf转crlf:
   ```
   git clone https://github.com/group/repo.git --config core.autocrlf=true 
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
