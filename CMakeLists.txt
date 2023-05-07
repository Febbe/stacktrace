cmake_minimum_required(VERSION 3.0)

set(MSVC_VERSION 1929)

project(fbbe__stacktrace LANGUAGES CXX C)

# All compilers support this now
# Also, when it's not allowed to use c++17 then a change to c++20 or c++23 will not happen as soon as all compiler and platforms support it.
# Therefore it might be better to just use another stacktrace library that supports c++14 and lower.
set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

add_library(stacktrace INTERFACE)
add_library(fbbe::stacktrace ALIAS stacktrace)

if (NOT DEFINED FBBE_USE_IMPL)
  if(MSVC) # MSVC or clang-cl or mingw/cygwin
    set(FBBE_USE_IMPL "msvc")
  else() # gcc or clang both support backtrace
    find_package(Backtrace REQUIRED)
    set(FBBE_USE_IMPL "itanium")
  endif()
  set(FBBE_USE_IMPL ${FBBE_USE_IMPL} CACHE STRING "The stacktrace implementation to use [msvc, gnu]")   
endif()

if(${FBBE_USE_IMPL} STREQUAL "msvc")
  add_library(stacktrace_msvc_impl STATIC msvc/src/msvc_stacktrace.cpp)
  target_include_directories(stacktrace_msvc_impl PUBLIC msvc/include)
  target_include_directories(stacktrace INTERFACE msvc/include)
  
  include(CheckIncludeFile)
  CHECK_INCLUDE_FILE(<stacktrace> has_stacktrace)
  message(STATUS "has <stacktrace> include: ${has_stacktrace}")
  # generator expr to exclude the msvc implementation when the msvc runtime has support for stacktraces
  target_link_libraries(stacktrace INTERFACE $<$<VERSION_LESS:$<CXX_COMPILER_VERSION>,23>:stacktrace_msvc_impl>)
elseif(${FBBE_USE_IMPL} STREQUAL "itanium") # gcc or clang support the itanium C++ unwind API
  target_include_directories(stacktrace INTERFACE itanium/include)
  find_package(Backtrace MODULE REQUIRED)
  # use generator expression hopefully it evalutes when the target is used and not when it is created
  # generator expression only for CMAKE_CXX_STANDARD < 23
  target_link_libraries(stacktrace INTERFACE $<$<VERSION_LESS:$<CXX_COMPILER_VERSION>,23>:Backtrace::backtrace>)
  # target_link_libraries(stacktrace INTERFACE $<$<VERSION_LESS:$<CXX_COMPILER_VERSION>,23>:${Backtrace_LIBRARIES}>)
  message("CMAKE_CXX_COMPILER_ID: ${CMAKE_CXX_COMPILER_ID}")
  if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    target_compile_options(stacktrace INTERFACE -fsized-deallocation)
  endif()
else()
  message (FATAL_ERROR "Unknown stacktrace implementation: ${FBBE_USE_IMPL}")
endif()

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