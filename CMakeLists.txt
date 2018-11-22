# ---------------------------------------------------------------------------
#
#   Author  : github.com/luncliff (luncliff@gmail.com)
#   Note
#       CMake support for project with LLVM toolchain 
#       GCC will be added ASAP when it supports coroutine
#   Support
#       - Clang + Windows
#       - Clang + MacOS
#       - Clang + Linux     (tested with Ubuntu and WSL)
#
# ---------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.5)

project(coroutine LANGUAGES CXX VERSION 1.2)

# list of cmake module path for this project
# list(APPEND CMAKE_MODULE_PATH   ${CMAKE_SOURCE_DIR}/cmake           )

# import cmake code snippets. see `cmake/`
include(cmake/alias.cmake)
include(cmake/test-cxx-coroutine.cmake)

if(NOT DEFINED CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
if(NOT IOS)
    set(BUILD_SHARED_LIBS true)
endif()

set(CMAKE_CXX_STANDARD 17)

# show project's build configuration
include(cmake/display-build-info.cmake)

if(${CMAKE_CXX_COMPILER_ID} MATCHES GNU)    # GCC
    message(FATAL_ERROR "Current version doesn't support GCC. Please use Clang 5.0+ or AppleClang")
    return(1)
endif()

# create coroutine helper library
add_library(${PROJECT_NAME} SHARED
    modules/${PLATFORM}/dllmain.cpp
    modules/${PLATFORM}/switch_to.cpp
    modules/${PLATFORM}/section.cpp
    modules/${PLATFORM}/wait_group.cpp
)
set_target_properties(${PROJECT_NAME} PROPERTIES 
VERSION
    ${PROJECT_VERSION}
)
target_include_directories(${PROJECT_NAME}
PUBLIC
    ${PROJECT_DIR}/interface
)
target_link_libraries(${PROJECT_NAME}
PUBLIC
    ${CMAKE_DL_LIBS}
)

# platform / compiler settings
if(WIN32)  # Clang + Windows
    target_sources(${PROJECT_NAME}
    PRIVATE
        modules/win32/clang.cpp
        modules/win32/thread_registry.cpp
        modules/win32/thread_message.cpp
    )
    target_compile_definitions(${PROJECT_NAME}
    PUBLIC
        _RESUMABLE_FUNCTIONS_SUPPORTED
    )
    if(BUILD_SHARED_LIBS)
        target_compile_definitions(${PROJECT_NAME}
        PRIVATE
            _WINDLL
        )
    endif()

    if(${CMAKE_CXX_COMPILER_ID} MATCHES Clang)
        # Argument for `clang-cl`
        #
        # `target_compile_options` removes duplicated -Xclang argument 
        # which must be protected. An alternative is to use CMAKE_CXX_FLAGS,
        # but the method will be used only when there is no way but to use it
        #
        # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Xclang -fcoroutines-ts")
        target_compile_options(${PROJECT_NAME} 
        PUBLIC
            /std:c++latest 
            -fms-compatibility 
            -Xclang -fcoroutines-ts
        PRIVATE
            -Wno-unused-function
            -Wno-c++98-compat 
            -Wno-reserved-id-macro 
            -Wno-missing-prototypes
        )
    elseif(MSVC)
        # ... support for MSVC ...
        message(WARNING "This cmake project is not for Windows/MSVC")
    endif()

    set_target_properties(${PROJECT_NAME} PROPERTIES 
        LINK_FLAGS "${LINK_FLAGS} /errorReport:send"
    )
else() # Clang + Non-Windows
    target_sources(${PROJECT_NAME}
    PRIVATE
        modules/thread/message.cpp
    )

    target_compile_options(${PROJECT_NAME} 
    PUBLIC
        -std=c++2a -fcoroutines-ts
        -fPIC 
    PRIVATE
        -Wall -Wno-unknown-pragmas
        -fno-rtti 
        -fvisibility=hidden
        -ferror-limit=5
    )

    if(CMAKE_BUILD_TYPE MATCHES Debug)
        # Generate coverage: disabled
        # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
        target_compile_options(${PROJECT_NAME}
        PUBLIC
            -g -O0 
        # PRIVATE
        #     --coverage
        )
    else()
        target_compile_options(${PROJECT_NAME}    
        PRIVATE
            -O3
        )
    endif()
endif()

if(APPLE)
    target_compile_options(${PROJECT_NAME} 
    PUBLIC
        -stdlib=libc++ 
    )
    target_link_libraries(${PROJECT_NAME}
    PUBLIC
        pthread
    )
elseif(LINUX)
    # linux-only source codes
    target_sources(${PROJECT_NAME}
    PRIVATE
        modules/linux/realtime.cpp
    )
    # expect libc++
    target_compile_options(${PROJECT_NAME} 
    PUBLIC
        -stdlib=libc++ 
    )
    target_link_libraries(${PROJECT_NAME}
    PUBLIC
        libc++.so libc++abi.so libc++experimental.a
        pthread rt
    )
endif()

# install settings

install(DIRECTORY       ${PROJECT_DIR}/interface/coroutine
        DESTINATION     ${CMAKE_INSTALL_PREFIX}/include
)
install(TARGETS         ${PROJECT_NAME}
        DESTINATION     ${CMAKE_INSTALL_PREFIX}/libs/${PLATFORM}
)

if(ANDROID OR IOS)
    return()
endif()

# testing

set(TEST_NAME ${PROJECT_NAME}_test)

add_executable(${TEST_NAME}
    test/test.entry.cpp
    test/generic/unplug.cpp
    test/generic/channel.cpp
    test/generic/generator.cpp
    test/generic/sequence.cpp

    # test/thread/message.cpp
    # test/thread/switch.cpp
)

target_include_directories(${TEST_NAME}
PRIVATE
    ${PROJECT_DIR}/test
)

target_link_libraries(${TEST_NAME}
PUBLIC
    ${PROJECT_NAME}
)

if(NOT WIN32)
    target_link_libraries(${TEST_NAME}
    PUBLIC
        stdc++
    )
endif()
