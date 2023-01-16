# how to build qt

## mac

1. 配置
   ```bash
   #!/bin/bash
   #
   # compile qt on mac
   
   bash_dir="$(cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
   qt_install_dir=${bash_dir}/prebuilt/qt_5.15.2
   qt_src_dir=${bash_dir}/qt-everywhere-src-5.15.2
   
   cd ${qt_src_dir}
   export LLVM_INSTALL_DIR=${HOME}/tools/clang+llvm-14.0.6-x86_64-apple-darwin 
   ./configure -prefix "${qt_install_dir}" -verbose -debug -force-debug-info -static -c++std c++17 -opensource -confirm-license -qt-zlib -qt-libpng -qt-webp -qt-libjpeg -qt-freetype  -no-opengl -skip qt3d -skip qtactiveqt -skip qtandroidextras -skip qtcharts -skip qtconnectivity -skip qtdatavis3d -skip qtdeclarative -skip qtdoc -skip qtgamepad -skip qtlocation -skip qtlottie -skip qtmultimedia -skip qtnetworkauth -skip qtpurchasing -skip qtquick3d -skip qtquickcontrols -skip qtquickcontrols2 -skip qtquicktimeline -skip qtremoteobjects -skip qtscript -skip qtsensors -skip qtspeech -skip qtsvg -skip qtwayland -skip qtwebglplugin -skip qtwebview -skip webengine -make libs -nomake examples -nomake tests
   make
   make install
   cd ${bash_dir}
   ```