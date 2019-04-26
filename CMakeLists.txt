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

project(coroutine LANGUAGES CXX VERSION 1.4.2)

# import cmake code snippets. see `cmake/`
list(APPEND CMAKE_MODULE_PATH
            ${PROJECT_SOURCE_DIR}/cmake
)
include(test-target-platform)
include(test-cxx-coroutine)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
include(report-project-info)

# GSL(Guideline Support Library) configuration
if(${CMAKE_TOOLCHAIN_FILE} MATCHES vcpkg.cmake)
    # portfile in vcpkg must give the path
else()
    # try find_package first
    find_package(GSL)
	if(GSL_FOUND)
		get_target_property(GSL_INCLUDE_DIR     GSL INTERFACE_INCLUDE_DIRECTORIES)
	else() # last chance. use submodule
        add_subdirectory(external/guideline)
		set(GSL_INCLUDE_DIR	${CMAKE_CURRENT_SOURCE_DIR}/external/guideline/include)
    endif()    
endif()

# ensure the path is defined
if(NOT DEFINED GSL_INCLUDE_DIR)
    message(FATAL_ERROR "Path - GSL_INCLUDE_DIR is required")
endif()

add_subdirectory(modules)

# build test project?
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

add_subdirectory(test)
add_dependencies(coroutine_test coroutine)
