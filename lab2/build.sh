#!/bin/bash

rm -rf build

mkdir build
cd build

cmake ..
make

cd ..

echo "Build finished! Check build"