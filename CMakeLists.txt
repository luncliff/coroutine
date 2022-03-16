cmake_minimum_required(VERSION 3.20)
project(coroutine LANGUAGES CXX VERSION 2.0.0)

option(BUILD_SHARED_LIBS    "https://cmake.org/cmake/help/latest/variable/BUILD_SHARED_LIBS.html" ON)
option(CMAKE_BUILD_TYPE     "https://cmake.org/cmake/help/latest/variable/CMAKE_BUILD_TYPE.html"  "Debug")
option(BUILD_TESTING        "Build test program" OFF)

# set(CMAKE_CXX_STANDARD 17)
include(GNUInstallDirs)

message(STATUS "system: ${CMAKE_SYSTEM}")
message(STATUS "build_type: ${CMAKE_BUILD_TYPE}")
message(STATUS "paths:")
message(STATUS " - ${CMAKE_INSTALL_PREFIX}")
message(STATUS " - ${CMAKE_CURRENT_SOURCE_DIR}")
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
find_package(Coroutines REQUIRED)
find_package(Threads REQUIRED) # Threads::Threads

# see https://github.com/microsoft/vcpkg
find_package(Microsoft.GSL 4.0 CONFIG REQUIRED) # Microsoft.GSL::GSL


string(TOLOWER ${CMAKE_SYSTEM_NAME} system_name)

list(APPEND headers
    src/coroutine/frame.h
    src/coroutine/${system_name}.h
)

add_library(coroutine
    ${headers}
    src/frame.cpp
    # src/${system_name}.cpp
)

set_target_properties(coroutine
PROPERTIES
    CXX_STANDARD 17
    PUBLIC_HEADER "${headers}"
    WINDOWS_EXPORT_ALL_SYMBOLS  true
)

target_include_directories(coroutine
PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(coroutine
PRIVATE
    Threads::Threads
)

# compiler options / macro hints
if(CMAKE_CXX_COMPILER_ID MATCHES Clang)
    if(WIN32)
        # 'target_compile_options' removes duplicated -Xclang directive.
        # avoide the removal using cmake flag variable
        target_compile_options(coroutine
        PUBLIC
            "/clang:-fcoroutines-ts"
            -fms-compatibility
        )
        # in this case, we always use portable header
        target_compile_definitions(coroutine
        PUBLIC
            # USE_PORTABLE_COROUTINE_HANDLE
            _RESUMABLE_FUNCTIONS_SUPPORTED
            # _EXPERIMENTAL_RESUMABLE_
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
    #         /std:c++latest
    #     )
    # else()
    #     target_compile_options(coroutine
    #     PUBLIC
    #         /std:c++17 /await
    #     )
    # endif()
    target_compile_options(coroutine
    PRIVATE
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

return()

#
# for testing, CTest will be used
#
if(NOT BUILD_TESTING)
    message(STATUS "Test is disabled.")
    return()
endif()
enable_testing()
find_package(Catch2 CONFIG REQUIRED) # Catch2::Catch2

# create_ctest( ... )
function(create_ctest TEST_NAME)
    # create a test exe with the given name ...
    add_executable(${TEST_NAME} test/${TEST_NAME}.cpp)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    target_link_libraries(${TEST_NAME}
    PRIVATE
        coroutine
    )
endfunction()

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

#
#   <coroutine/net.h>
#


#
#   <coroutine/channel.hpp>
#
if(CMAKE_CXX_COMPILER_ID MATCHES GNU)
    message(WARNING "using gcc: the compiler may not work for current code")
endif()
