#!/usr/bin/sh
if [ ! -e build ] || [ "$1" = "-c" ]; then
    mkdir -p build
    cd build
    cmake ..
    cd ..
fi
if [ "$1" = "-b" ]; then
    cmake --build build --target bench --config Release
    ./build/bench/bench
else
    cmake --build build --target test
    ./build/test/test
fi
