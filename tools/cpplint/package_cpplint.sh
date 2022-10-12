#!/bin/bash
#
# Perform bash tests.

bash_dir="$(cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

python2_dir=/c/Python27
python2_pip_dir=${python2_dir}/Scripts

export PATH=${python2_dir}:${python2_pip_dir}:$PATH

python --version
if [[ $? != 0 ]]; then
  echo "Need python"
  exit 1
fi

pyinstaller --version
if [[ $? != 0 ]]; then
  echo "Install pyinstaller"

  pip --version
  if [[ $? != 0 ]]; then
    echo "Need pip"
    exit 1
  fi

  # Upgrading pip
  #python -m pip install --upgrade pip

  # Install the Pyinstaller Package, have to install an older version that has compatibility with 2.7
  pip install pyinstaller==3.6

  exit 1
fi

cpplint_script=${bash_dir}/src/cpplint.py
cpplint_dist_dir=${bash_dir}/dist
cpplint_spec_dir=${cpplint_dist_dir}
pyinstaller --specpath ${cpplint_spec_dir} --distpath ${cpplint_dist_dir} --onefile ${cpplint_script}
if [[ $? != 0 ]]; then
  echo "package cpplint failed"
  exit 1
fi
