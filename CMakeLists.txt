#
#   Author  : github.com/luncliff (luncliff@gmail.com)
#   Note
#       CMake support for non-VisualStudio projects
#
#   Support
#       - MSVC  + Windows   (Visual Studio 2017 or later)
#       - Clang + Windows   (Ninja)
#       - Clang + MacOS     (Unix Makefiles, Ninja)
#       - Clang + Linux     (Unix Makefiles, Ninja) : WSL, Ubuntu 16.04 and later
#
#   Working
#       - GCC   + Linux     (Ninja, Unix Makefiles)
#
#   Variables
#       - TEST_DISABLED : Explicitly disables building tests
#
cmake_minimum_required(VERSION 3.8)

project(coroutine 
        LANGUAGES CXX 
        VERSION 1.5.0
)

if(NOT DEFINED BUILD_SHARED_LIBS)
    set(BUILD_SHARED_LIBS true)
endif()
if(NOT DEFINED CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

#
# import cmake code snippets. see 'cmake/'
#
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
include(check-compiler)
include(report-project-info)

#
# import external libraries. see 'external/'
#
include(external-gsl)

#
# build/interface targets for CMake. see 'modules/'
#
set(BUILD_INTERFACE_DIR ${PROJECT_SOURCE_DIR}/interface)
add_subdirectory(modules/portable)

if(${CMAKE_CXX_COMPILER_ID} MATCHES GNU)
    # WIP: perform test with the portable headers
else()
    add_subdirectory(modules/event)     # coroutine_event
    add_subdirectory(modules/net)       # coroutine_net
    add_subdirectory(modules/thread)    # coroutine_thread
endif()

#
# export declared cmake targets
#
# 'coroutine-targets' is indeed better name, but without using 'configure_file()'
# the exporting step will be more complicated for non-CMake users.
# just merge all contents into the file 'coroutine-config.cmake'
#
install(EXPORT      coroutine-config
        # NAMESPACE   coro::
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/coroutine
)

#
# generate/install config & version info
#
include(CMakePackageConfigHelpers)
set(VERSION_FILE_PATH   ${CMAKE_BINARY_DIR}/cmake/coroutine-config-version.cmake)
write_basic_package_version_file(${VERSION_FILE_PATH}
    VERSION             ${PROJECT_VERSION}
    COMPATIBILITY       SameMajorVersion
)
install(FILES           ${VERSION_FILE_PATH} 
        DESTINATION     ${CMAKE_INSTALL_PREFIX}/share/coroutine
)

#
# for testing, CTest will be used
#
if(TEST_DISABLED)
    message(STATUS "Test is disabled.")
    return()
elseif(NOT ${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
    message(STATUS "This is not a root project. Skipping the tests")
    return()
elseif(ANDROID OR IOS)
    return()
endif()

enable_testing()

# add_subdirectory(modules/concrt)    # concurrency helper for test codes
# add_subdirectory(test)
