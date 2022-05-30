if(TARGET std::coroutines)
    # This module has already been processed. Don't do it again.
    return()
endif()

include(CMakePushCheckState)
include(CheckIncludeFileCXX)
include(CheckCXXCompilerFlag)
include(CheckCXXSourceCompiles)
# include(CheckCXXSymbolExists)

cmake_push_check_state()

#
# Acquire informations about current build environment. Especially for Compiler & STL
#   - support_latest
#   - support_coroutine
#   - support_intrinsic_builtin: optional. compiler supports `__builtin_coro_*`
#   - has_coroutine
#   - has_coroutine_ts
#
if(CMAKE_CXX_COMPILER_ID MATCHES Clang)
    if(WIN32)
        check_cxx_compiler_flag("/std:c++latest"            support_latest)
        check_cxx_compiler_flag("/clang:-fcoroutines-ts"    support_coroutine)
        check_include_file_cxx("experimental/coroutine" has_coroutine_ts
            "/std:c++latest"
        )
    else()
        check_cxx_compiler_flag("-std=c++2a"        support_latest)
        check_cxx_compiler_flag("-fcoroutines-ts"   support_coroutine)
        check_include_file_cxx("experimental/coroutine" has_coroutine_ts
            "-std=c++2a"
        )
    endif()

elseif(MSVC)
    #
    # Notice that `/std:c++latest` and `/await` is exclusive to each other.
    # With MSVC, we have to distinguish Coroutines TS & C++ 20 Coroutines
    #
    check_cxx_compiler_flag("/std:c++20"    support_cpp20)
    check_cxx_compiler_flag("/await"        support_coroutine)
    check_include_file_cxx("coroutine"  has_coroutine
        "/std:c++latest"
    )
    if(NOT has_coroutine)
        message(STATUS "Try <expeirmental/coroutine> (Coroutines TS) instead of <coroutine> ...")
        check_include_file_cxx("experimental/coroutine" has_coroutine_ts
            "/std:c++17"
        )
    endif()
    # has coroutine headers?
    if(NOT has_coroutine AND NOT has_coroutine_ts)
        message(FATAL_ERROR "There are no headers for C++ Coroutines")
    endif()

elseif(CMAKE_CXX_COMPILER_ID MATCHES GNU)
    #
    # expect GCC 10 or later
    #
    check_cxx_compiler_flag("-std=gnu++20"        support_cpp20)
    check_cxx_compiler_flag("-fcoroutines"        support_coroutine)
    check_include_file_cxx("coroutine" has_coroutine
        "-std=gnu++20 -fcoroutines"
    )
    if(APPLE)
        # -isysroot "/usr/local/Cellar/gcc/${CMAKE_CXX_COMPILER_VERSION}/include/c++/${CMAKE_CXX_COMPILER_VERSION}"
        # -isysroot "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include"
        # -isysroot "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/include/c++/v1"
    endif()
    set(try_compile_flags "-fcoroutines")
endif()
# support compiler options for coroutine?
if(NOT support_coroutine)
    message(FATAL_ERROR "The compiler doesn't support C++ Coroutines")
endif()

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# support `__builtin_coro_*`?
get_filename_component(intrinsic_test_cpp
                       test/support_intrinsic_builtin.cpp ABSOLUTE)
try_compile(support_intrinsic_builtin ${CMAKE_BINARY_DIR}/supports
    SOURCES         ${intrinsic_test_cpp}
    # This is a workaround. for some reasone CMAKE_FLAGS has no effect.
    # Instead, place the compiler option where macro definition are placed
    COMPILE_DEFINITIONS ${try_compile_flags}    
    CXX_STANDARD    20                          
    OUTPUT_VARIABLE report
)
if(NOT support_intrinsic_builtin)
    message(STATUS ${report})
endif()
message(STATUS "supports __builtin_coro: ${support_intrinsic_builtin}")

# support `__builtin_noop_coroutine`?

add_library(std::coroutine IMPORTED INTERFACE)
# set_target_properties(std::coroutine
# PROPERTIES
#     # ...
# )
