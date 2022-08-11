

add_executable(test tests.cpp
    test_enum.cpp
    test_macros.cpp    
    test_fmt.cpp    
    test_span.cpp
    test_svector.cpp)
    
target_include_directories(test PRIVATE ../src)

if(MSVC)
    target_compile_options(test PRIVATE /W4 /WX /Zc:preprocessor)
else()
    target_compile_options(test PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
