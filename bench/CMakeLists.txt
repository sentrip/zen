# Disable the Google Benchmark requirement on Google Test
set(BENCHMARK_ENABLE_TESTING NO)

include(FetchContent)

FetchContent_Declare(
    googlebenchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG origin/master
)

FetchContent_MakeAvailable(googlebenchmark)

add_executable(bench bench.cpp 
    bench_fmt.cpp)

    target_include_directories(bench PRIVATE ../src)
target_link_libraries(bench benchmark::benchmark)

if ("${CMAKE_SYSTEM_NAME}" MATCHES "Windows")
    target_link_libraries(bench Shlwapi)
endif()
