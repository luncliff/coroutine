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
cmake_minimum_required(VERSION 3.8)

project(coroutine LANGUAGES CXX VERSION 1.3.2)

# import cmake code snippets. see `cmake/`
list(APPEND CMAKE_MODULE_PATH 
            ${PROJECT_SOURCE_DIR}/cmake
)
include(test-target-platform)
include(test-cxx-coroutine)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
include(display-build-info)

add_subdirectory(external/guideline)
add_subdirectory(modules)

# ---------------------------------------------------------------------------

if(IOS OR ANDROID)
    message(STATUS "Mobile cross build doesn't support tests")
    return()
endif()

if(NOT ${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
    message(STATUS "This is not a root project. Skip tests")
    return()
endif()

add_subdirectory(test)
add_dependencies(coroutine_test coroutine)
