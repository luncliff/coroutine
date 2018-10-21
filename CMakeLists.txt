# ---------------------------------------------------------------------------
#
#   Author
#       luncliff@gmail.com
#
#   Note
#       CMake for project with LLVM toolchain 
#
# ---------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.1)

# Variable setup
if(WIN32 AND MSVC)
    message(FATAL_ERROR "This cmake project is not for Windows/MSVC")
    return(1) 
endif()

project(coroutine LANGUAGES CXX VERSION 1.1.0)

set(ROOT_DIR    ${CMAKE_SOURCE_DIR})
set(PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

if(NOT IOS)
    SET(BUILD_SHARED_LIBS true)
endif()

# Project Info

message(STATUS "========== ${PROJECT_NAME} : ${PROJECT_VERSION} ==========")
message(STATUS "Root        \t: ${ROOT_DIR}")
message(STATUS "Project     \t: ${PROJECT_DIR}")
message(STATUS "Install     \t: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Build Type  \t: ${CMAKE_BUILD_TYPE}")

message(STATUS "System      \t: ${CMAKE_SYSTEM}")
message(STATUS "Compiler")
message(STATUS " - ID       \t: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS " - Version  \t: ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS " - Path     \t: ${CMAKE_CXX_COMPILER}")

message(FATAL_ERROR "Not supported")
