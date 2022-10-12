#!/bin/bash
#
# Init dev env.

bash_dir="$(cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

echo "init.sh begin..."

repo_dir="${bash_dir}"

#package_cpplint_script=${repo_dir}/liveandlearn/tools/cpplint/package_cpplint.sh

#bash ${package_cpplint_script}

# install pre-commit
echo "install pre-commit"
cp -f ${repo_dir}/git/hooks/pre-commit ${repo_dir}/.git/hooks/

echo "init.sh end!"
