cmake_minimum_required(VERSION 3.0)

project(fbbe__stacktrace LANGUAGES CXX)
set (CMAKE_CXX_STANDARD 17) # All compilers support this now

add_library(stacktrace STATIC)
add_library(fbbe::stacktrace ALIAS stacktrace)
target_include_directories(stacktrace PUBLIC win/include)
target_sources(stacktrace PRIVATE win/src/msvc_stacktrace.cpp)

# Compilation tests

add_executable(test_stacktrace_17 test/main.cpp)
target_compile_features(test_stacktrace_17 PRIVATE cxx_std_17)
target_link_libraries(test_stacktrace_17 PRIVATE fbbe::stacktrace)

add_executable(test_stacktrace_20 test/main.cpp)
target_compile_features(test_stacktrace_20 PRIVATE cxx_std_20)
target_link_libraries(test_stacktrace_20 PRIVATE fbbe::stacktrace)

add_executable(test_stacktrace_23 test/main.cpp)
target_compile_features(test_stacktrace_23 PRIVATE cxx_std_23)
target_link_libraries(test_stacktrace_23 PRIVATE fbbe::stacktrace)

enable_testing()
add_test(test_17 test_stacktrace_17)
add_test(test_20 test_stacktrace_20)
add_test(test_23 test_stacktrace_23)