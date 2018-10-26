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
#       - iPhone OS
#       - Android NDK
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

# show project's build configuration
include(display-build-info)

if(${CMAKE_CXX_COMPILER_ID} MATCHES GNU)    # GCC
    message(FATAL_ERROR "Current version doesn't support GCC. Please use Clang 5.0+ or AppleClang")
    return(1)
endif()

# create coroutine helper library for target platform
add_library(${PROJECT_NAME} SHARED
    ${PROJECT_DIR}/modules/${PLATFORM}/dllmain.cpp
)

# general settings
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
            -std=c++2a
            -fcoroutines-ts
            -fPIC 
        PRIVATE
            -Wall
            -fno-rtti 
            -fvisibility=hidden
            -ferror-limit=5
    )

    if(CMAKE_BUILD_TYPE MATCHES Debug)
        # Generate coverage
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage -g -O0")
        target_compile_options(${PROJECT_NAME}
        PUBLIC
            -g -O0 
        PRIVATE
            --coverage  # do not propagate this coverage option
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
elseif(LINUX)
    # will use prebuilt packages
    target_compile_options(${PROJECT_NAME}
    PUBLIC
        -nostdinc++ -nodefaultlibs
    )

    # see: libcxx_prebuilt-config.cmake
    find_package(libcxx_prebuilt CONFIG REQUIRED 
        PATHS ${PROJECT_DIR}/prebuilt
    )
    target_include_directories(${PROJECT_NAME} 
    PUBLIC
        ${LIBCXX_INCLUDE_DIR}
    )
    target_link_libraries(${PROJECT_NAME}
    PUBLIC
        ${LIBCXX_LIBS}
    )

    # target_include_directories(${PROJECT_NAME} 
    # PUBLIC
    #     ${PROJECT_DIR}/prebuilt/include/c++/v1/
    # )
    # link_directories(
    #     ${PROJECT_DIR}/prebuilt/lib
    # )
    # target_link_libraries(${PROJECT_NAME}
    # PUBLIC
    #     libc++.so
    #     libc++experimental.a
    # )
endif()

# install settings

install(DIRECTORY       ${PROJECT_DIR}/interface/coroutine
        DESTINATION     ${CMAKE_INSTALL_PREFIX}/include
)
install(TARGETS         ${PROJECT_NAME}
        DESTINATION     ${CMAKE_INSTALL_PREFIX}/libs
)

if(ANDROID OR IOS)
    return()
endif()

# testing

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
    ${PROJECT_NAME}
)
