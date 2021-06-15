#!/bin/bash
set -e

qmake -r
make -j$(nproc)
sudo make install

cd SNCPython/SNCPythonLib
./build.sh
cd ../..


