#!/bin/bash
#

bash_dir="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

$bash_dir/localfortuneserver/generate_qt_vs_project.bat
$bash_dir/localfortuneclient/generate_qt_vs_project.bat

echo "success!"
