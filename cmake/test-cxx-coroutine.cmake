#
#   Author  : github.com/luncliff (luncliff@gmail.com)
#   Note
#       Check if the compiler supports coroutine
#
cmake_policy(VERSION 3.5)

if(TARGET cxx-coroutine-helper)    # prevent multiple import
    return()
endif()

include(CheckCXXCompilerFlag)

if(MSVC)
    check_cxx_compiler_flag(/std:c++latest  cxx_latest      )
    check_cxx_compiler_flag(/await          cxx_coroutine   )
elseif(${CMAKE_CXX_COMPILER_ID} MATCHES Clang)
    check_cxx_compiler_flag(-std=c++2a      cxx_latest      )
    check_cxx_compiler_flag(-fcoroutines-ts cxx_coroutine   )
else()
    # elseif(${CMAKE_CXX_COMPILER_ID} MATCHES GNU)    # GCC
    message(WARNING "${CMAKE_CXX_COMPILER_ID} is not checkd by the author")
endif()

#end