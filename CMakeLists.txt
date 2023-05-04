cmake_minimum_required(VERSION 3.0)

project(fbbe__stacktrace LANGUAGES CXX)

# All compilers support this now
# Also, when it's not allowed to use c++17 then a change to c++20 or c++23 will not happen as soon as all compiler and platforms support it.
# Therefore it might be better to just use another stacktrace library that supports c++14 and lower.
set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_EXTENSIONS OFF)

add_library(stacktrace INTERFACE)
add_library(fbbe::stacktrace ALIAS stacktrace)


if (NOT DEFINED FBBE_USE_IMPL)
  if(MSVC) # MSVC or clang-cl or mingw/cygwin
    set(FBBE_USE_IMPL "msvc")
  elseif(WIN32)
    find_package(Backtrace REQUIRED)
    set(FBBE_USE_IMPL "gnu")  
  elseif(GNU OR UNIX OR APPLE) 
    set(FBBE_USE_IMPL "gnu")
  endif()
  set(FBBE_USE_IMPL ${FBBE_USE_IMPL} CACHE STRING "The stacktrace implementation to use [msvc, gnu]")   
endif()

if(${FBBE_USE_IMPL} STREQUAL "msvc")
  add_library(stacktrace_msvc_impl STATIC)
  target_include_directories(stacktrace_msvc_impl PUBLIC msvc/include)
  target_include_directories(stacktrace INTERFACE msvc/include)
  target_sources(stacktrace_msvc_impl PRIVATE msvc/src/msvc_stacktrace.cpp)
  include(CheckIncludeFile)
  CHECK_INCLUDE_FILE(<stacktrace> has_stacktrace)
  # generator expr to exclude the msvc implementation when the msvc runtime has support for stacktraces
  target_link_libraries(stacktrace INTERFACE <$OR<$<NOT:$<BOOL:${has_stacktrace}>>:<$<NOT:$<BOOL:${MSVC}>>:stacktrace_msvc_impl>)
elseif(${FBBE_USE_IMPL} STREQUAL "gnu")
  #add_library(stacktrace_gnu_impl STATIC)
  #target_include_directories(stacktrace_gnu_impl PUBLIC gcc/include)
  target_include_directories(stacktrace INTERFACE gcc/include)
  find_package(Backtrace REQUIRED)
  target_include_directories(stacktrace INTERFACE ${Backtrace_INCLUDE_DIRS})
  target_link_libraries(stacktrace INTERFACE ${Backtrace_LIBRARIES} ${Backtrace_LIBRARY} ${CMAKE_DL_LIBS})
  message(STATUS "Backtrace_INCLUDE_DIRS: ${Backtrace_INCLUDE_DIRS}")
  message(STATUS "Backtrace_LIBRARIES: ${Backtrace_LIBRARIES}")
  message(STATUS "Backtrace_LIBRARY: ${Backtrace_LIBRARY}")
  message(STATUS "CMAKE_DL_LIBS: ${CMAKE_DL_LIBS}")
  #target_sources(stacktrace_msvc_impl PRIVATE gcc/src/gcc_stacktrace.cpp)
  #target_link_libraries(stacktrace INTERFACE stacktrace_gnu_impl)
  message (STATUS "GNU")
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