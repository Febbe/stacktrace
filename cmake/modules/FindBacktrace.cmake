cmake_minimum_required(VERSION 3.15)

if(TARGET Backtrace::backtrace)
  return()
endif()

include(CMakePushCheckState)
include(CheckSymbolExists)
include(FindPackageHandleStandardArgs)

list(APPEND _backtrace_search_paths ${CMAKE_SYSTEM_LIBRARY_PATH})
list(APPEND _backtrace_search_paths ${CMAKE_C_IMPLICIT_LINK_DIRECTORIES})
list(APPEND _backtrace_search_paths ${CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES})
list(REMOVE_DUPLICATES _backtrace_search_paths)

list(APPEND _backtrace_include_paths ${CMAKE_SYSTEM_INCLUDE_PATH})
list(APPEND _backtrace_include_paths ${CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES})
list(APPEND _backtrace_include_paths ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
list(REMOVE_DUPLICATES _backtrace_include_paths)

add_library(backtrace INTERFACE IMPORTED)
# List of variables to be provided to find_package_handle_standard_args()
#set(_Backtrace_STD_ARGS Backtrace_INCLUDE_DIR)

if(Backtrace_HEADER)
  set(_Backtrace_HEADER_TRY "${Backtrace_HEADER}")
else(Backtrace_HEADER)
  set(_Backtrace_HEADER_TRY "backtrace.h")
endif(Backtrace_HEADER)

#message("_backtrace_include_paths: ${_backtrace_include_paths}")
find_path(Backtrace_INCLUDE_DIR "${_Backtrace_HEADER_TRY}" PATHS ${_backtrace_include_paths})
set(Backtrace_INCLUDE_DIRS ${Backtrace_INCLUDE_DIR})
#message("Backtrace_INCLUDE_DIR: ${Backtrace_INCLUDE_DIR}")


if (NOT DEFINED Backtrace_LIBRARY)
  message("check if we already have backtrace_create_state, e.g., in libc")
  # First, check if we already have backtrace(), e.g., in libc
  cmake_push_check_state(RESET)
  set(CMAKE_REQUIRED_INCLUDES ${Backtrace_INCLUDE_DIRS})
  set(CMAKE_REQUIRED_QUIET ${Backtrace_FIND_QUIETLY})
  check_symbol_exists("backtrace_create_state" "${_Backtrace_HEADER_TRY}" _Backtrace_SYM_FOUND)
  cmake_pop_check_state()

  if(_Backtrace_SYM_FOUND)
    # Avoid repeating the message() call below each time CMake is run.
    if(NOT Backtrace_FIND_QUIETLY AND NOT DEFINED Backtrace_LIBRARY)
      message("backtrace facility detected in default set of libraries")
    endif()
    set(Backtrace_LIBRARY "" CACHE FILEPATH "Library providing libbacktrace, empty for default set of libraries")
  endif()
endif()

if (NOT DEFINED Backtrace_LIBRARY AND NOT _Backtrace_SYM_FOUND)
  message("Backtrace_LIBRARY not found, searching in compiler implicit defined paths")
  find_library(Backtrace_LIBRARY "execinfo" "backtrace" "libbacktrace.a" NAMES PATHS ${_backtrace_search_paths} )
  # Prepend list with library path as it's more common practice
  set(_Backtrace_STD_ARGS Backtrace_LIBRARY ${_Backtrace_STD_ARGS}) 
endif()

if((Backtrace_LIBRARY OR _Backtrace_SYM_FOUND) AND NOT Backtrace_INCLUDE_DIR )
  message("backtrace facility detected in ${Backtrace_LIBRARY}, but header not found")
endif()
if(NOT Backtrace_LIBRARY AND NOT _Backtrace_SYM_FOUND)
  message("BACKTRACE_LIBRARY not found, downloading libbacktrace")
  include(ExternalProject)
  ExternalProject_Add(libbacktrace
    PREFIX "${CMAKE_CURRENT_BINARY_DIR}/libbacktrace"
    GIT_REPOSITORY https://github.com/ianlancetaylor/libbacktrace.git
    GIT_TAG master
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
    CONFIGURE_COMMAND ../libbacktrace/configure --prefix=${CMAKE_CURRENT_BINARY_DIR}/libbacktrace
    BUILD_COMMAND make 
    INSTALL_COMMAND make install
  )
  ExternalProject_Get_Property(libbacktrace install_dir)
  set(_Backtrace_HEADER_TRY "${install_dir}/include/backtrace.h")  
  set(Backtrace_LIBRARY "${install_dir}/lib/libbacktrace.a")
  set(Backtrace_INCLUDE_DIR "${install_dir}/include")
  file(MAKE_DIRECTORY ${Backtrace_INCLUDE_DIR})
  
  add_dependencies(backtrace libbacktrace)
endif()

set(Backtrace_LIBRARIES ${Backtrace_LIBRARY})
set(Backtrace_HEADER "${_Backtrace_HEADER_TRY}" CACHE STRING "Header providing libbacktrace facility")

target_include_directories(backtrace INTERFACE ${Backtrace_INCLUDE_DIR})
target_link_libraries(backtrace INTERFACE ${Backtrace_LIBRARIES})
target_compile_options(stacktrace INTERFACE -g)
target_link_options(stacktrace INTERFACE -rdynamic)
add_library(Backtrace::backtrace ALIAS backtrace)
set(Backtrace_TARGET Backtrace::backtrace)

find_package_handle_standard_args(Backtrace FOUND_VAR Backtrace_FOUND REQUIRED_VARS Backtrace_TARGET)
mark_as_advanced(Backtrace_HEADER Backtrace_INCLUDE_DIR Backtrace_LIBRARY)
