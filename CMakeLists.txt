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

project(coroutine LANGUAGES CXX VERSION 1.3)

# import cmake code snippets. see `cmake/`
list(APPEND CMAKE_MODULE_PATH
            ${CMAKE_CURRENT_SOURCE_DIR}/cmake
)
include(test-target-platform)
include(test-cxx-coroutine)

if(NOT DEFINED CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# show project's build configuration
include(display-build-info)

# import external projects
add_subdirectory(external/guideline)

# delegate the build jobs to sub-directories
add_subdirectory(modules)

# ---------------------------------------------------------------------------
if(IOS OR ANDROID)
    return()
endif()

add_subdirectory(test)
add_dependencies(coroutine_test coroutine)
