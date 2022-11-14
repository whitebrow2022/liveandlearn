#!/bin/bash
#
# Invoke node main.js

bash_dir="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

node_modules_dir="${bash_dir}/node_modules"

cd ${bash_dir}

if [[ ! -d ${node_modules_dir} ]]; then 
  # Start installing node deps as a job
  npm install &
  deps_pid=$!
  # Wait until Node deps is done
  wait ${deps_pid}
fi

node main.js
