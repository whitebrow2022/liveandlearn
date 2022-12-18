# bash函数

1. 函数参数
   ```bash
   #!/bin/bash
   #
   # 函数参数
   
   #######################################
   # 检查函数参数.
   # Arguments:
   #   可变参数列表
   #######################################
   function check_param {
     echo The fun location is $0
     echo There are $# arguments
     # It's ok to not quote internal integer variables.
     if (( $# > 0 )); then
       echo "Argument 1 is $1"
     fi
     if (( $# > 1 )); then
       echo "Argument 2 is $2"
     fi
     if (( $# > 0 )); then
       echo "<$@>" and "<$*>" are the same.
       echo List the elements in a for loop to see the difference!
       echo "* gives:"
       for arg in "$*"; do echo "<$arg>"; done
       echo "@ gives:"
       for arg in "$@"; do echo "<$arg>"; done
     fi
   }
   echo Test check_param fun 1:
   # Invoking functions
   check_param
   echo Test check_param fun 2:
   check_param "hello" "world"
   ```

2. 函数返回值
   ```bash
   #!/bin/bash
   #
   # 函数返回值
   
   #######################################
   # 检查函数返回值.
   # Arguments:
   #   random base
   # Returns:
   #   random value.
   #######################################
   function generate_random {
     # echo There are $# arguments
     local random_base=10
     if (($# > 0)); then
       random_base=$1
     fi
     local random_value=$((1 + $RANDOM % ${random_base}))
     # return
     echo ${random_value}
   }
   random_value=$(generate_random 100)
   echo generate_random 100 is ${random_value}
   ```
