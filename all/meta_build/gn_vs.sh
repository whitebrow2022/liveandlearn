#!/bin/bash
#
#

bash_dir="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

repo_dir="${bash_dir}/../.."

gn="${repo_dir}/tools/gn/mac/gn"
out_dir="${bash_dir}/out"
gn_args="is_debug=true is_official_build=false"

${gn} gen "${out_dir}" --ide=vs --sln=meta_build --args="${gn_args}" 