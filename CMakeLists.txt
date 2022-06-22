cmake_minimum_required(VERSION 3.21)
project(coroutine LANGUAGES CXX VERSION 2.0.0)

option(BUILD_SHARED_LIBS    "https://cmake.org/cmake/help/latest/variable/BUILD_SHARED_LIBS.html" ON)
option(CMAKE_BUILD_TYPE     "https://cmake.org/cmake/help/latest/variable/CMAKE_BUILD_TYPE.html"  "Debug")
option(BUILD_TESTING        "Build test program" OFF)
option(WITH_GCD             "Build with GCD(libdispatch)" OFF)

# set(CMAKE_C_STANDARD 17)
# set(CMAKE_CXX_STANDARD 20)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS true)
include(GNUInstallDirs)

message(STATUS "system: ${CMAKE_SYSTEM}")
message(STATUS "build_type: ${CMAKE_BUILD_TYPE}")
message(STATUS "paths:")
message(STATUS " - ${CMAKE_INSTALL_PREFIX}")
message(STATUS " - ${PROJECT_SOURCE_DIR}")
message(STATUS " - ${PROJECT_BINARY_DIR}")
message(STATUS)
message(STATUS "compiler:")
message(STATUS " - ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS " - ${CMAKE_CXX_COMPILER}")
message(STATUS)
message(STATUS "cmake:")
message(STATUS " - ${CMAKE_VERSION}")
message(STATUS " - ${CMAKE_COMMAND}")
message(STATUS " - ${CMAKE_TOOLCHAIN_FILE}")
message(STATUS " - ${CMAKE_GENERATOR}")
message(STATUS " - ${CMAKE_BUILD_TOOL}")
message(STATUS)

# get_filename_component(CMAKE_COROUTINES_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/FindCoroutines.cmake ABSOLUTE)
# if(NOT EXISTS ${CMAKE_COROUTINES_MODULE_PATH})
#     file(DOWNLOAD https://raw.githubusercontent.com/facebookexperimental/libunifex/main/cmake/FindCoroutines.cmake
#                 ${CMAKE_COROUTINES_MODULE_PATH}
#         EXPECTED_HASH SHA256=0129305dd3d030de21684543fec9b1e7b0af7a67ed1bad4348c3259ecfb20cef
#     )
# endif()

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

#
# Known STL-Compiler issues
#   - https://github.com/microsoft/STL, issue 100
#
if(CMAKE_CXX_COMPILER_ID MATCHES Clang AND WIN32)
    message(WARNING "clang-cl won't work with <experimental/coroutine>")
endif()
# find_package(Coroutines REQUIRED)
find_package(Threads REQUIRED) # Threads::Threads
find_package(fmt CONFIG REQUIRED) # fmt::fmt-header-only
find_package(spdlog CONFIG REQUIRED) # spdlog::spdlog_header_only
# see https://github.com/microsoft/GSL/tree/v4.0.0
# check https://github.com/microsoft/vcpkg ms-gsl
# find_package(Microsoft.GSL 4.0 CONFIG REQUIRED) # Microsoft.GSL::GSL

add_library(coro_cpp17 src/coro.hpp src/coro.cpp)
target_compile_options(coro_cpp17 PUBLIC /std:c++17 /await PRIVATE /Zc:__cplusplus)

add_library(coro_cpp20 src/coro.hpp src/coro.cpp)
target_compile_options(coro_cpp20 PUBLIC /std:c++20 PRIVATE /Zc:__cplusplus)

add_library(coro_latest src/coro.hpp src/coro.cpp)
target_compile_options(coro_latest PUBLIC /std:c++latest PRIVATE /Zc:__cplusplus)

add_library(coro_action17 SHARED src/action.hpp src/action.cpp)
target_compile_options(coro_action17 PRIVATE /Zc:__cplusplus)
target_link_libraries(coro_action17 PRIVATE coro_cpp17)

add_library(coro_action20 SHARED src/action.hpp src/action.cpp)
target_compile_options(coro_action20 PRIVATE /Zc:__cplusplus)
target_link_libraries(coro_action20 PRIVATE coro_cpp20)

return()
set(CMAKE_CXX_STANDARD 20)
string(TOLOWER ${CMAKE_SYSTEM_NAME} system_name)

list(APPEND headers
    src/coroutine/frame.h
    src/coroutine/${system_name}.h
)

add_library(coroutine
    ${headers}
    # src/frame.cpp
    # src/${system_name}.cpp
    src/libmain.cpp
)

set_target_properties(coroutine
PROPERTIES
    PUBLIC_HEADER "${headers}"
    WINDOWS_EXPORT_ALL_SYMBOLS true
    CXX_STANDARD_REQUIRED true
)

target_include_directories(coroutine
PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(coroutine
PRIVATE
    Threads::Threads
    fmt::fmt-header-only
    spdlog::spdlog_header_only
)

# compiler options / macro hints
if(CMAKE_CXX_COMPILER_ID MATCHES Clang)
    if(WIN32)
        # 'target_compile_options' removes duplicated -Xclang directive.
        # avoide the removal using cmake flag variable
        target_compile_options(coroutine
        PUBLIC
            "/clang:-std=c++20" # c++2b for experiment?
        PRIVATE
            "/clang:-fms-compatibility-version=1900"
            # -Wall -Wextra -ferror-limit=5
        )

        # clang 13: `__cpp_coroutines` macro is 201703L. `__cpp_lib_coroutine` is NOT defined
        target_compile_definitions(coroutine
        PUBLIC
            # force feature activation.
            # see https://github.com/llvm/llvm-project/blob/release/13.x/clang/lib/Frontend/InitPreprocessor.cpp
            __cpp_impl_coroutine=201902L 
            # make <yvals_core.h> define __cpp_lib_coroutine
            # see https://github.com/microsoft/STL, issue 1730, 2510
            _DOWNLEVEL_COROUTINES_SUPPORTED
        )

    elseif(UNIX OR APPLE)
        target_compile_options(coroutine
        PUBLIC
            -stdlib=libc++
            -fcoroutines-ts
        )
    endif()

elseif(CMAKE_CXX_COMPILER_ID MATCHES GNU)
    target_compile_options(coroutine
    PUBLIC
        -fcoroutines
    )
    target_link_libraries(coroutine
    PUBLIC
        stdc++
    )

elseif(MSVC)
    # select between C++ Coroutines TS & C++ 20 Coroutines
    # if(support_intrinsic_builtin AND has_coroutine)
    #     target_compile_options(coroutine
    #     PUBLIC
    #         /std:c++20
    #     )
    # else()
    #     target_compile_options(coroutine
    #     PUBLIC
    #         /std:c++17 /await
    #     )
    # endif()

    # see https://docs.microsoft.com/en-us/cpp/build/reference/zc-cplusplus
    target_compile_options(coroutine
    PRIVATE
        /Zc:__cplusplus
        /JMC-
    )

endif()

# compile definition and linkage
if(WIN32)
    target_compile_definitions(coroutine
    PRIVATE
        WIN32_LEAN_AND_MEAN NOMINMAX
    )

elseif(CMAKE_SYSTEM_NAME MATCHES Linux)
    if(ANDROID)
        target_link_libraries(coroutine
        PUBLIC
            ${ANDROID_STL} # expect c++_shared
        )
    else()
        target_link_libraries(coroutine
        PUBLIC
            rt
        )
    endif()

elseif(UNIX OR APPLE)
    target_link_libraries(coroutine
    PUBLIC
        c++
    )

endif()

install(TARGETS coroutine
        EXPORT  coroutine-config
        PUBLIC_HEADER   DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

#
# export declared cmake targets
#
# 'coroutine-targets' is indeed better name, but without using 'configure_file()'
# the exporting step will be more complicated for non-CMake users.
# just merge all contents into the file 'coroutine-config.cmake'
#
install(EXPORT      ${PROJECT_NAME}-config
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME} # share/coroutine
)

#
# generate/install config & version info
#
include(CMakePackageConfigHelpers)
set(VERSION_FILE_PATH   ${CMAKE_BINARY_DIR}/cmake/${PROJECT_NAME}-config-version.cmake)
write_basic_package_version_file(${VERSION_FILE_PATH}
    VERSION             ${PROJECT_VERSION}
    COMPATIBILITY       SameMajorVersion
)
install(FILES           ${VERSION_FILE_PATH} 
        DESTINATION     ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME} # share/coroutine
)

#
# for testing, CTest will be used
#
if(NOT BUILD_TESTING)
    message(STATUS "Test is disabled.")
    return()
endif()
enable_testing()
find_package(Catch2 2.13 CONFIG REQUIRED) # Catch2::Catch2

# create a test exe with the given name ...
add_executable(coroutine_test_suite
    test/test_main.cpp
    test/test_action.cpp
)

set_target_properties(coroutine_test_suite
PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED true
)

# target_compile_definitions(coroutine_test_suite
# PRIVATE
#     CATCH_CONFIG_FAST_COMPILE
# )

target_link_libraries(coroutine_test_suite
PRIVATE
    Catch2::Catch2 coroutine
)

if(coroutine_IS_TOP_LEVEL)
    install(TARGETS coroutine_test_suite DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

add_test(NAME test_exception COMMAND coroutine_test_suite "[exception]")

# create_ctest( article_russian_roulette  coroutine_portable )

#
#   <coroutine/yield.hpp>
#

#
#   <coroutine/frame.h>
#   <coroutine/return.h>
#

#
#   <coroutine/windows.h>
#   <coroutine/unix.h>, <coroutine/linux.h>
#   <coroutine/pthread.h>
#
if(APPLE)
add_test(NAME test_dispatch COMMAND coroutine_test_suite "[dispatch]")
endif()

#
#   <coroutine/net.h>
#


#
#   <coroutine/channel.hpp>
#
if(CMAKE_CXX_COMPILER_ID MATCHES GNU)
    message(WARNING "using gcc: the compiler may not work for current code")
endif()
