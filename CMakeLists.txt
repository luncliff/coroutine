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

if(WIN32 AND MSVC)
    message(FATAL_ERROR "This cmake project is not for Windows/MSVC")
    return(1) 
endif()

project(coroutine LANGUAGES CXX VERSION 1.2)

# list of cmake module path for this project
list(APPEND CMAKE_MODULE_PATH   ${CMAKE_SOURCE_DIR}/cmake           )
list(APPEND CMAKE_MODULE_PATH   ${CMAKE_CURRENT_SOURCE_DIR}/cmake   )

# import cmake code snippets. see cmake/
include(alias)
include(test-cxx-coroutine)

if(NOT DEFINED CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# show project's build configuration
include(display-build-info)

if(${CMAKE_CXX_COMPILER_ID} MATCHES GNU)    # GCC
    message(FATAL_ERROR "Current version doesn't support GCC. Please use Clang 5.0+ or AppleClang")
    return(1)
endif()

# create coroutine helper library
add_library(${PROJECT_NAME} SHARED
    ${PROJECT_DIR}/modules/${PLATFORM}/dllmain.cpp
    ${PROJECT_DIR}/modules/${PLATFORM}/switch_to.cpp
    ${PROJECT_DIR}/modules/${PLATFORM}/section.cpp
    ${PROJECT_DIR}/modules/${PLATFORM}/wait_group.cpp
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
    set_target_properties(${PROJECT_NAME} PROPERTIES 
        LINK_FLAGS "${LINK_FLAGS} /errorReport:send"
    )
else() # Clang + Non-Windows
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
        # Generate coverage
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
        target_compile_options(${PROJECT_NAME}
        PUBLIC
            -g -O0 
        PRIVATE
            --coverage
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
        ${PROJECT_DIR}/modules/linux/realtime.cpp
    )
    # expect libc++
    target_compile_options(${PROJECT_NAME} 
    PUBLIC
        -stdlib=libc++ 
    )
    target_link_libraries(${PROJECT_NAME}
    PUBLIC
        c++ c++abi c++experimental
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

set(TEST_NAME ${PROJECT_NAME}_test_suite)

add_executable(${TEST_NAME}
    ${PROJECT_DIR}/test/test.entry.cpp
    ${PROJECT_DIR}/test/test.sequence.cpp
    ${PROJECT_DIR}/test/test.unplug.cpp
)

target_include_directories(${TEST_NAME}
PRIVATE
    ${PROJECT_DIR}/test
)

target_link_libraries(${TEST_NAME}
PUBLIC
    ${PROJECT_NAME}
)

if(NOT MSVC)
    target_compile_options(${TEST_NAME}
    PRIVATE
        -g --coverage
    )
endif()
