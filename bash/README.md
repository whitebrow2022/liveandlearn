# bash

- [style guide](https://google.github.io/styleguide/shellguide.html)

## while loop

1. 复制源文件到100个副本
   ```bash
   #!/bin/bash
   #
   #
   
   bash_dir="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
   # 复制源文件到100个副本
   x=1
   maxnum=100
   filename="README"
   fileext=".md"
   src_file=${bash_dir}/${filename}${fileext}
   dest_dir=${bash_dir}/tmp
   if [[ ! -d ${dest_dir} ]]; then
     mkdir -p ${dest_dir}
   fi
   while [ ${x} -le ${maxnum} ]; do
     # echo "loop ${x} times"
     dest_file="${dest_dir}/${filename}${x}${fileext}"
     echo "cp -f ${src_file} ${dest_file}"
     cp -f ${src_file} ${dest_file}
     # increase
     x=$((${x} + 1))
   done
   ```

### 子文档

- [函数](function.md)
