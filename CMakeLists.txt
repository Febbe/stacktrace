cmake_minimum_required(VERSION 3.0)

project(fbbe__stacktrace LANGUAGES CXX C)

if (PROJECT_IS_TOP_LEVEL)
  set (CMAKE_CXX_STANDARD 17)
  set (CMAKE_CXX_STANDARD_REQUIRED ON)
  set (CMAKE_CXX_EXTENSIONS OFF)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

add_library(stacktrace INTERFACE)
add_library(fbbe::stacktrace ALIAS stacktrace)

if (NOT DEFINED FBBE_USE_IMPL)
  if(MSVC) # MSVC or clang-cl using MSVC runtime/STL
    set(FBBE_USE_IMPL "msvc")
  elseif(NOT (WIN32 OR MINGW)) # gcc or clang both support backtrace, which utilizes the itanium C++ unwind API
    find_package(Backtrace MODULE REQUIRED)
    set(FBBE_USE_IMPL "itanium")
  else()
    set(FBBE_USE_IMPL "windows")
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
elseif(${FBBE_USE_IMPL} STREQUAL "windows") 
  add_library(stacktrace_win_impl STATIC windows/src/msvc_stacktrace.cpp)
  target_include_directories(stacktrace_win_impl PUBLIC windows/include)
  target_include_directories(stacktrace INTERFACE windows/include)
  target_link_libraries(stacktrace INTERFACE $<$<VERSION_LESS:$<CXX_COMPILER_VERSION>,23>:stacktrace_win_impl>)
  target_compile_options(stacktrace INTERFACE -g)
  target_link_options(stacktrace INTERFACE -g -Wl,--pdb=)
else()
  message (FATAL_ERROR "Unknown stacktrace implementation: ${FBBE_USE_IMPL}")
endif()

if (PROJECT_IS_TOP_LEVEL)

# Compilation tests
enable_testing()

add_executable(test_stacktrace_17 test/main.cpp)
target_compile_features(test_stacktrace_17 PRIVATE cxx_std_17)
target_link_libraries(test_stacktrace_17 PRIVATE fbbe::stacktrace)
add_test(test_17 test_stacktrace_17)

list(FIND CMAKE_CXX_COMPILE_FEATURES cxx_std_20 _cxx_std_20_index)
if(NOT _cxx_std_20_index EQUAL -1)
  add_executable(test_stacktrace_20 test/main.cpp)
  target_compile_features(test_stacktrace_20 PRIVATE cxx_std_20)
  target_link_libraries(test_stacktrace_20 PRIVATE fbbe::stacktrace)
  add_test(test_20 test_stacktrace_20)
endif()

list(FIND CMAKE_CXX_COMPILE_FEATURES cxx_std_23 _cxx_std_23_index)
if(NOT _cxx_std_23_index EQUAL -1)
  add_executable(test_stacktrace_23 test/main.cpp)
  target_compile_features(test_stacktrace_23 PRIVATE cxx_std_23)
  target_link_libraries(test_stacktrace_23 PRIVATE fbbe::stacktrace)
  add_test(test_23 test_stacktrace_23)
endif()
endif()