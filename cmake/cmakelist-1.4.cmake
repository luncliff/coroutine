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
cmake_minimum_required(VERSION 3.8) # c++ 17

project(coroutine LANGUAGES CXX VERSION 1.4.3)

# import cmake code snippets. see `cmake/`
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
include(check-target-platform)
include(check-compiler)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
include(report-project-info)

# Dependency configuration
if(${CMAKE_TOOLCHAIN_FILE} MATCHES vcpkg.cmake)
    # portfile in vcpkg must give the path
    set(GSL_INCLUDE_DIR	${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include)
else()
    # use submodule
    add_subdirectory(external/guideline)
	set(GSL_INCLUDE_DIR	${CMAKE_CURRENT_SOURCE_DIR}/external/guideline/include)
endif()

# ensure the path is defined
if(NOT DEFINED GSL_INCLUDE_DIR)
    message(FATAL_ERROR "Path - GSL_INCLUDE_DIR is required")
else()
    message(STATUS "GSL (C++ Core Guideline Support Library)")
    message(STATUS "  Path      \t: ${GSL_INCLUDE_DIR}")
    message(STATUS)
endif()

# Project modules
add_subdirectory(modules)

# Test projects
if(TEST_DISABLED)
    message(STATUS "Test is disabled.")
    return()
elseif(IOS OR ANDROID)
    message(STATUS "Mobile cross build doesn't support tests")
    return()
elseif(NOT ${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
    message(STATUS "This is not a root project. Skip tests")
    return()
endif()

# we will use ctest. so enable it
enable_testing() 
add_subdirectory(test)
