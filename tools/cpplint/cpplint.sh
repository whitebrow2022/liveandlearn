#!/bin/bash
#
# Perform bash tests.

bash_dir="$(cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

# repo_dir="${bash_dir}/../.."
repo_dir="${bash_dir}/../.."

cpplint_script=${bash_dir}/src/cpplint.py
cpplint_filters='-build/include,-whitespace/comments,-whitespace/indent'
cppcode_dir="${repo_dir}"

if [[ "$OSTYPE" == "msys" ]]; then
  python2_path=/c/Python27/python.exe
else
  python2_path=python2
fi

# cpplint_cmd=${bash_dir}/dist/cpplint
cpplint_cmd="${python2_path} ${cpplint_script}"
echo "cpplint cmd: '${cpplint_cmd}'"

for fullfile in $(find ${cppcode_dir} -type f); do
  if [[ ${fullfile} == *"/.git/"* ]]; then
    continue
  fi
  if [[ ${fullfile} == *"/node_modules/"* ]]; then
    continue
  fi
  if [[ ${fullfile} == *"/microsoft_cpp/"* ]]; then
    continue
  fi
  if [[ ${fullfile} == *"/build/"* ]]; then
    continue
  fi
  if [[ ${fullfile} == *"/macos/"*.h ]]; then
    continue
  fi
  if [[ ${fullfile} == *"/ios/"*.h ]]; then
    continue
  fi 

  filename=$(basename -- "${fullfile}")
  if [[ "${filename}" == "resource.h" ]]; then
    continue
  fi
  if [[ "${filename}" == "stdafx.h" ]]; then
    continue
  fi
  extension="${filename##*.}"
  if [[ "${extension}" == "h" || "${extension}" == "cc" ]]; then
    # use python2 script
    # ${python2_path} ${cpplint_script} --linelength=80 --filter="${cpplint_filters}" ${fullfile}
    # or use exe 
    ${cpplint_cmd} --linelength=80 --filter="${cpplint_filters}" ${fullfile}
  fi
done
