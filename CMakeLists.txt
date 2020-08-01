#
#   github.com/luncliff (luncliff@gmail.com)
#
cmake_minimum_required(VERSION 3.12) # supports CXX_STANDARD 20
project(coroutine	LANGUAGES   CXX 
                    VERSION     1.5.1
)
if(NOT DEFINED BUILD_SHARED_LIBS)
    set(BUILD_SHARED_LIBS true)
endif()
if(NOT DEFINED CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
message(STATUS "System: ${CMAKE_SYSTEM}")
message(STATUS)
message(STATUS "Paths:")
message(STATUS " - ${PROJECT_SOURCE_DIR}")
message(STATUS " - ${CMAKE_INSTALL_PREFIX}")
message(STATUS " - ${CMAKE_SOURCE_DIR}")
message(STATUS " - ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS)
message(STATUS "Compiler")
message(STATUS " - ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS " - ${CMAKE_CXX_COMPILER}")
message(STATUS)
message(STATUS "CMake")
message(STATUS "  - ${CMAKE_VERSION}")
message(STATUS "  - ${CMAKE_COMMAND}")
message(STATUS "  - ${CMAKE_TOOLCHAIN_FILE}")
message(STATUS)
message(STATUS "Tools")
message(STATUS " - ${CMAKE_GENERATOR}")
message(STATUS " - ${CMAKE_BUILD_TOOL}")
message(STATUS)

#
# import external libraries. see 'external/'
#
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
find_package(Microsoft.GSL CONFIG)
if(Microsoft.GSL_FOUND)
    get_target_property(GSL_INCLUDE_DIR 
                        Microsoft.GSL::GSL INTERFACE_INCLUDE_DIRECTORIES)
else()
    add_subdirectory(external/ms-gsl)
    get_filename_component(GSL_INCLUDE_DIR 
                           ${PROJECT_SOURCE_DIR}/external/ms-gsl/include ABSOLUTE)
endif()
message(STATUS "using ms-gsl: ${GSL_INCLUDE_DIR}")
message(STATUS)

#
# Acquire informations about current build environment. Especially for Compiler & STL
#   - support_latest
#   - support_coroutine
#   - has_coroutine
#   - has_coroutine_ts
#
include(CheckIncludeFileCXX)
include(CheckCXXCompilerFlag)
include(CheckSymbolExists)
if(CMAKE_CXX_COMPILER_ID MATCHES Clang)
    if(WIN32)
        check_cxx_compiler_flag("/std:c++latest"    support_latest)
        check_cxx_compiler_flag("/clang:-fcoroutines-ts" support_coroutine)
    else()
        check_cxx_compiler_flag("-std=c++2a"          support_latest)
        check_cxx_compiler_flag("-fcoroutines-ts"     support_coroutine)
    endif()

    check_include_file_cxx("experimental/coroutine" has_coroutine_ts    -std=c++2a)

elseif(MSVC)
    check_cxx_compiler_flag("/std:c++latest"    support_latest)
    check_cxx_compiler_flag("/await"            support_coroutine)
    check_include_file_cxx("coroutine"  has_coroutine   /std:c++latest)
    if(NOT has_coroutine)
        message(STATUS "Try <expeirmental/coroutine> (Coroutines TS) instead of <coroutine> ...")
        check_include_file_cxx("experimental/coroutine" has_coroutine_ts    /std:c++17)
    endif()
    if(NOT has_coroutine AND NOT has_coroutine_ts)
        message(FATAL_ERROR "the compiler doesn't support C++ Coroutines")
    endif()

elseif(CMAKE_CXX_COMPILER_ID MATCHES GNU)
    check_cxx_compiler_flag("-std=gnu++2a"        support_latest)
    check_cxx_compiler_flag("-fcoroutines"        support_coroutine)
endif()
if(NOT support_coroutine)
    message(FATAL_ERROR "The compiler doesn't support C++ Coroutines")
endif()

#
# Known STL-Compiler issues
#   - https://github.com/microsoft/STL, issue 100
#
if(CMAKE_CXX_COMPILER_ID MATCHES Clang AND WIN32)
    message(WARNING "clang-cl won't work with <experimental/coroutine>")
    set(need_portable true)
endif()

# set(CMAKE_CXX_STANDARD  20)
set(MODULE_INTERFACE_DIR ${PROJECT_SOURCE_DIR}/interface)
add_subdirectory(modules/portable)	# corotuien_portable
add_subdirectory(modules/system)    # coroutine_system
add_subdirectory(modules/net)       # coroutine_net

install(FILES           ${MODULE_INTERFACE_DIR}/coroutine/frame.h
                        ${MODULE_INTERFACE_DIR}/coroutine/return.h
                        ${MODULE_INTERFACE_DIR}/coroutine/channel.hpp
                        ${MODULE_INTERFACE_DIR}/coroutine/yield.hpp
        DESTINATION     ${CMAKE_INSTALL_PREFIX}/include/coroutine
)
if(WIN32)
    install(FILES       ${MODULE_INTERFACE_DIR}/coroutine/windows.h
            DESTINATION ${CMAKE_INSTALL_PREFIX}/include/coroutine
    )

elseif(CMAKE_SYSTEM_NAME MATCHES Linux)
    install(FILES       ${MODULE_INTERFACE_DIR}/coroutine/linux.h
                        ${MODULE_INTERFACE_DIR}/coroutine/pthread.h
            DESTINATION ${CMAKE_INSTALL_PREFIX}/include/coroutine
    )

elseif(UNIX OR APPLE)
    install(FILES       ${MODULE_INTERFACE_DIR}/coroutine/unix.h
                        ${MODULE_INTERFACE_DIR}/coroutine/pthread.h
            DESTINATION ${CMAKE_INSTALL_PREFIX}/include/coroutine
    )

endif()
install(FILES			${MODULE_INTERFACE_DIR}/coroutine/net.h
        DESTINATION     ${CMAKE_INSTALL_PREFIX}/include/coroutine
)

install(TARGETS     coroutine_portable coroutine_system coroutine_net
        EXPORT      coroutine-config
        INCLUDES    DESTINATION     ${CMAKE_INSTALL_PREFIX}/include
        RUNTIME     DESTINATION     ${CMAKE_INSTALL_PREFIX}/bin
        LIBRARY     DESTINATION     ${CMAKE_INSTALL_PREFIX}/lib
        ARCHIVE     DESTINATION     ${CMAKE_INSTALL_PREFIX}/lib
)

#
# export declared cmake targets
#
# 'coroutine-targets' is indeed better name, but without using 'configure_file()'
# the exporting step will be more complicated for non-CMake users.
# just merge all contents into the file 'coroutine-config.cmake'
#
install(EXPORT      ${PROJECT_NAME}-config
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}
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
        DESTINATION     ${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}
)

#
# for testing, CTest will be used
#
if(NOT BUILD_TESTING)
    message(STATUS "Test is disabled.")
    return()
elseif(NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    message(STATUS "This is not a root project. Skipping the tests")
    return()
elseif(ANDROID OR IOS)
    return()
endif()
enable_testing()

# helper for test codes
set(BUILD_TESTING OFF)
add_subdirectory(external/latch)    
add_subdirectory(external/sockets)

add_subdirectory(test)
