#!/bin/bash
#

bash_dir="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

$bash_dir/generate_vs2022_project.bat

echo "success!"
