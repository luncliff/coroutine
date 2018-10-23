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

if(WIN32 AND MSVC)
    message(FATAL_ERROR "This cmake project is not for Windows/MSVC")
    return(1) 
endif()

project(coroutine LANGUAGES CXX VERSION 1.1.1)

# Variable setup

set(ROOT_DIR    ${CMAKE_SOURCE_DIR})
set(PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

if(ANDROID)
    set(PLATFORM ${ANDROID_ABI})
elseif(IOS OR IOS_DEPLOYMENT_TARGET)
    set(PLATFORM ios)
elseif(${CMAKE_SYSTEM} MATCHES Windows)
    set(PLATFORM win32)
elseif(${CMAKE_SYSTEM} MATCHES Darwin)
    set(OSX true)
    set(PLATFORM osx)
elseif(${CMAKE_SYSTEM} MATCHES Linux)
    set(LINUX true)
    set(PLATFORM linux)
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

if(${CMAKE_CXX_COMPILER_ID} MATCHES GNU)    # GCC
    message(FATAL_ERROR "Current version doesn't support GCC. Please use Clang 5.0+ or AppleClang")
    return(1)
endif()

# module: coroutine library for target platform
set(MODULE_NAME ${PROJECT_NAME}_${PLATFORM})

add_library(${MODULE_NAME}
    ${PROJECT_DIR}/modules/${PLATFORM}/dllmain.cpp
)

target_include_directories(${MODULE_NAME}
PUBLIC
    ${PROJECT_DIR}/interface
)

target_link_libraries(${MODULE_NAME}
PUBLIC
    ${CMAKE_DL_LIBS}
)

if(${CMAKE_CXX_COMPILER_ID} MATCHES Clang)
    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag(-std=c++2a          cxx_20          )
    check_cxx_compiler_flag(-fcoroutines-ts     cxx_coroutine   )
    # check_cxx_compiler_flag(-fmodules-ts        cxx_module      )

    target_compile_options(${MODULE_NAME}
    PUBLIC
        -std=c++2a
        -fcoroutines-ts -fPIC
    PRIVATE
        -Wall
        -fvisibility=hidden -fno-rtti # -fmodules-ts
    )
    if(CMAKE_BUILD_TYPE MATCHES Debug)
        # Generate coverage with Clang
        # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage -g -O0")
        target_compile_options(${MODULE_NAME}
        PUBLIC
            -g -O0 
        # PRIVATE
        #     --coverage  # do not propagate this coverage option
        )
    else()
        target_compile_options(${MODULE_NAME}    
        PRIVATE
            -O3
        )
    endif()
endif()

if(APPLE)       # AppleClang
    target_compile_options(${MODULE_NAME} 
    PUBLIC
        -stdlib=libc++ 
    )
endif()

install(DIRECTORY       ${PROJECT_DIR}/interface/coroutine
        DESTINATION     ${CMAKE_INSTALL_PREFIX}/include
)
install(TARGETS         ${MODULE_NAME}
        DESTINATION     ${CMAKE_INSTALL_PREFIX}/libs
)

if(ANDROID OR IOS)
    return()
endif()

set(TEST_NAME ${PROJECT_NAME}_test_suite)

add_executable(${TEST_NAME}
    ${PROJECT_DIR}/test/test.entry.cpp
)

target_include_directories(${TEST_NAME}
PRIVATE
    ${PROJECT_DIR}/test
)

target_link_libraries(${TEST_NAME}
PUBLIC
    ${MODULE_NAME}
)
