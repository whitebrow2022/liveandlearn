#!/bin/bash
#
# Generate 'macOS' project for OpencvWrapper.

bash_dir="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

echo "generate macos command line project ..."

osx_proj_dir="$bash_dir/build/macos/examples/opencv_wrapper_example"
cmake -S "$bash_dir/examples/opencv_wrapper_example" -B "$osx_proj_dir" -G "Xcode" -DMACOS:BOOL=ON

example_xcworkspace="$bash_dir/MacExample.xcodeproj"
if ! [ -h "$example_xcworkspace" ]; then
  ln -s "$osx_proj_dir/opencv_wrapper_example.xcodeproj" "$example_xcworkspace"
fi

echo "success!"
