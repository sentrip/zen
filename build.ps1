if (!(Test-Path -Path "build") -or ($args[0] -eq "-c")) {
    if (!(Test-Path -Path "build")) {
        mkdir build
    }
    cd build
    cmake ..
    cd ..
}
if ($args[0] -eq "-b") {
    cmake --build build --target bench --config Release
    .\build\bench\Release\bench.exe
} else {
    cmake --build build --target test
    .\build\test\Debug\test.exe
}