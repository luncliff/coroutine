#
#   Author  : github.com/luncliff (luncliff@gmail.com)
#   Note
#       Check if the compiler supports coroutine
#
cmake_policy(VERSION 3.8)

include(CheckCXXCompilerFlag)
if(${CMAKE_CXX_COMPILER_ID} MATCHES Clang)
    check_cxx_compiler_flag(-std=c++2a      cxx_latest      )
    check_cxx_compiler_flag(-fcoroutines-ts cxx_coroutine   )

elseif(MSVC)
    check_cxx_compiler_flag(/std:c++latest  cxx_latest      )
    check_cxx_compiler_flag(/await          cxx_coroutine   )

elseif(${CMAKE_CXX_COMPILER_ID} MATCHES GNU)
    check_cxx_compiler_flag(-std=gnu++2a    cxx_latest      )
    check_cxx_compiler_flag(-fcoroutines    cxx_coroutine   )
    check_cxx_compiler_flag(-fconcepts      cxx_concepts   )

endif()
