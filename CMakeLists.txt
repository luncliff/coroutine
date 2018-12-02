# ---------------------------------------------------------------------------
#
#   Author  : github.com/luncliff (luncliff@gmail.com)
#   Note
#       CMake support for project with LLVM toolchain 
#       GCC will be added ASAP when it supports coroutine
#
#   Support
#       - MSVC  + Windows   (Visual Studio)
#       - Clang + Windows   (Ninja)
#       - Clang + MacOS     (Unix Makefiles)
#       - Clang + Linux     (Unix Makefiles. WSL, Ubuntu 1604 and later)
#
# ---------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.5)

project(coroutine LANGUAGES CXX VERSION 1.2)

# add external libraries
# add_subdirectory(external/guideline)

# import cmake code snippets. see `cmake/`
include(cmake/alias.cmake)
include(cmake/test-cxx-coroutine.cmake)

if(NOT DEFINED CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
if(NOT IOS)
    set(BUILD_SHARED_LIBS true)
endif()

# show project's build configuration
include(cmake/display-build-info.cmake)

if(${CMAKE_CXX_COMPILER_ID} MATCHES GNU)    # GCC
    message(FATAL_ERROR "Current version doesn't support GCC. Please use Clang 5.0+ or AppleClang")
    return(1)
endif()

add_subdirectory(modules)

if(IOS OR ANDROID)
    return()
endif()

add_subdirectory(test)
add_dependencies(coroutine_test coroutine)
