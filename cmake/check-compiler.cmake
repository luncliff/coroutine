#
#   Author  : github.com/luncliff (luncliff@gmail.com)
#   Note
#       Check if the compiler supports coroutine
#
cmake_policy(VERSION 3.8)

# Compiler flag for C++ coroutines
include(CheckCXXCompilerFlag)
if(${CMAKE_CXX_COMPILER_ID} MATCHES Clang)
    check_cxx_compiler_flag(-std=c++2a      cxx_latest      )
    check_cxx_compiler_flag(-fcoroutines-ts cxx_coroutine   )
elseif(MSVC)
    check_cxx_compiler_flag(/std:c++latest  cxx_latest      )
    check_cxx_compiler_flag(/await          cxx_coroutine   )
elseif(${CMAKE_CXX_COMPILER_ID} MATCHES GNU)    # GCC
    message(FATAL_ERROR "Current version doesn't support GCC")
endif()

# test compiler flag for C++ coroutines
