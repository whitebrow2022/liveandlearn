#!/bin/bash
#

bash_dir="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

chromium_src_path="${bash_dir}/../.."
build_output_path="${chromium_src_path}/liveandlearn/chromium/out"
# win x86
#build_dir_name="chromium-test-win-x86"
#gen_params="target_cpu=\"x86\" com_init_check_hook_disabled=true"
# win x64
build_dir_name="chromium-test-win-x64"
gen_params="target_cpu=\"x64\" com_init_check_hook_disabled=true"
build_dir="${build_output_path}"
# 生成debug还是release
build_config="release"
build_dir="${build_dir}/${build_dir_name}-${build_config}"
if [ "${build_config}" = "debug" ]; then
  gen_params="${gen_params} is_debug=true symbol_level=2"
else
  # gen_params="${gen_params} is_component_build=false is_debug=false"
  gen_params="${gen_params} is_component_build=false is_debug=true symbol_level=2 use_debug_fission=true"
fi

echo ${build_dir}
echo ${gen_params}

cd ${chromium_src_path}
gn.bat gen --ide=vs --filters="//liveandlearn/*" "${build_dir}" --args="${gen_params}" --sln="chromium_test"
