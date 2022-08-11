#!/usr/bin/sh
if [ "$1" = "-b" ]; then
    mkdir -p build
    cd build
    cmake ..  -DCMAKE_BUILD_TYPE=Release
    cd ..
    cmake --build build --target benchmark
    cmake --build build --target bench
    ./build/bench/bench
else
    if [ ! -e build ] || [ "$1" = "-c" ]; then
        mkdir -p build
        cd build
        cmake ..
        cd ..
    fi
    cmake --build build --target test
    ./build/test/test
fi
