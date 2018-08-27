# ---------------------------------------------------------------------------
#
#   Author
#       luncliff@gmail.com
#
#   Note
#       CMake for non-windows project
#
# ---------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.1)

# Global setup

if(WIN32 AND MSVC)
    message(FATAL_ERROR "This cmake project is not for Windows/MSVC")
    return(1) 
else()
    add_compile_options(-Wall -fexceptions)
endif()

# Project Info

project(cppmagic LANGUAGES CXX 
        VERSION 1.0.1
)

message(STATUS "========== ${PROJECT_NAME} : ${PROJECT_VERSION} ==========")

# Alias for known variables
set(ROOT_DIR    ${CMAKE_SOURCE_DIR})
set(PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
message(STATUS "Root        \t: ${ROOT_DIR}")
message(STATUS "Project     \t: ${PROJECT_DIR}")

# set(CMAKE_INSTALL_PREFIX ${PROJECT_DIR}/install)
message(STATUS "Install     \t: ${CMAKE_INSTALL_PREFIX}")

if(NOT CMAKE_BUILD_TYPE)
    # set(CMAKE_BUILD_TYPE Release)
    set(CMAKE_BUILD_TYPE Debug)
endif()
message(STATUS "Build Type  \t: ${CMAKE_BUILD_TYPE}")

# System / Compiler

if(ANDROID)
    set(PLATFORM ${ANDROID_ABI})
elseif(IOS OR IOS_DEPLOYMENT_TARGET)
    set(PLATFORM ios)
elseif(${CMAKE_SYSTEM} MATCHES Windows)
    set(PLATFORM windows)
elseif(${CMAKE_SYSTEM} MATCHES Darwin)
    set(OSX true)
    set(PLATFORM osx)
elseif(${CMAKE_SYSTEM} MATCHES Linux)
    set(LINUX true)
    set(PLATFORM linux)
endif()

if(ANDROID OR WIN32)
    SET(BUILD_SHARED_LIBS true)
endif()

message(STATUS "System      \t: ${CMAKE_SYSTEM}")
message(STATUS "Compiler")
message(STATUS " - ID       \t: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS " - Path     \t: ${CMAKE_CXX_COMPILER}")

if(NOT IOS)
    link_directories(
        ${PROJECT_DIR}/prebuilt/lib
    )
endif()
if(LINUX)
    link_directories(
        ${PROJECT_DIR}/libcxxabi/lib
    )
endif()

# [cppmagic]: Core features

add_library(${PROJECT_NAME}
    ${PROJECT_DIR}/msvc/dllmain.cpp
)
set_target_properties(${PROJECT_NAME} PROPERTIES 
    VERSION         ${PROJECT_VERSION}
    # CXX_STANDARD    17      # this property is equal to `-std=gnu++17`
    #                         # We will use `-std=c++1z` instead
)

# Platform-specific configurations

if(ANDROID)
    message(STATUS "Android")
    message(STATUS "  Platform\t: ${ANDROID_PLATFORM}")     # Android-21 ++
    message(STATUS "  Arch    \t: ${ANDROID_ARCH_NAME}")    # arm
    message(STATUS "  ABI     \t: ${ANDROID_ABI}")          # armeabi-v7a or else
    message(STATUS "  NDK Path\t: ${ANDROID_NDK}")          # Path/to/NDK
    message(STATUS "  STL     \t: ${ANDROID_STL}")          # expect c++_shared

    target_compile_options(${PROJECT_NAME}
        PRIVATE
            -fstack-protector-strong
    )   

    # <asm/types.h> 를 찾는다. 현재는 armv7, armv8, x86에 대해서만. 
    if(${ANDROID_ABI} STREQUAL "arm64-v8a")
        target_include_directories(${PROJECT_NAME}
            PRIVATE
                ${ANDROID_NDK}/sysroot/usr/include/aarch64-linux-android
        )
    elseif(${ANDROID_ABI} MATCHES "arm")
        target_include_directories(${PROJECT_NAME}
            PRIVATE
                ${ANDROID_NDK}/sysroot/usr/include/arm-linux-androideabi
        )
    elseif(${ANDROID_ABI} MATCHES "x86") # x86, x86_64
        target_include_directories(${PROJECT_NAME}
            PRIVATE
                ${ANDROID_NDK}/sysroot/usr/include/x86_64-linux-android
        )
    endif()

    # STL: GNU Shared
    if(${ANDROID_STL} STREQUAL "gnustl_shared")
        target_include_directories(${PROJECT_NAME}
            PRIVATE
                ${ANDROID_NDK}/sources/cxx-stl/gnu-libstdc++/include/ 
        )
        # link_directories(
        #     ${ANDROID_NDK}/sources/cxx-stl/gnu-libstdc++/4.9/libs/${ANDROID_ABI}/
        # )
    # STL: C++ Shared
    elseif(${ANDROID_STL} STREQUAL "c++_shared")
        target_include_directories(${PROJECT_NAME}
            PRIVATE
                ${ANDROID_NDK}/sources/cxx-stl/llvm-libc++/include/ 
        )
        # link_directories(
        #     ${ANDROID_NDK}/sources/cxx-stl/llvm-libc++/libs/${ANDROID_ABI}/
        # )
    endif()

    target_include_directories(${PROJECT_NAME}
        PRIVATE
            # JNI
            ${ANDROID_NDK}/sysroot/usr/include/ 
            ${ANDROID_NDK}/sources/android/support/include/
            # ABI
            ${ANDROID_NDK}/platforms/${ANDROID_PLATFORM}/arch-${ANDROID_ARCH_NAME}/usr/include/ 
            # JNI declarations
            ${PROJECT_DIR}/android/jni/
    )

    # link_directories(
    #     # ABI
    #     ${ANDROID_NDK}/platforms/${ANDROID_PLATFORM}/arch-${ANDROID_ARCH_NAME}/usr/lib/    
    #     # Nested libraries
    #     ${PROJECT_DIR}/android/jniLibs/${ANDROID_ABI}
    # )

    target_sources(${PROJECT_NAME}
        PRIVATE
            ${PROJECT_DIR}/android/jni/adapter.h
            ${PROJECT_DIR}/android/jni/adapter.cpp
    )
endif() # Android

target_include_directories(${PROJECT_NAME} 
    PUBLIC
        ${PROJECT_DIR}
    PRIVATE    
        ${PROJECT_DIR}/clang/
)

if(WIN32) # Clang + Windows
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

    # argument from `clang-cl -help`
    # `target_compile_options` removes duplicated -Xclang argument which must be protected
    # An alternative is to use CMAKE_CXX_FLAGS like CMake 2.x versons.
    #   But the method will be used only when there is no way but to use it
    target_compile_options(${PROJECT_NAME} 
        PUBLIC
            /std:c++latest 
            # /Gr
            -fms-compatibility 
            -Xclang -fcoroutines-ts
            # -Xclang -std=c++2a
            -Wno-c++98-compat 
            -Wno-reserved-id-macro 
            -Wno-missing-prototypes
    )
else() # Clang + Non-Windows
    target_compile_options(${PROJECT_NAME} 
        PUBLIC
            -std=c++1z
            -fcoroutines-ts
            -fno-rtti 
            -fPIC 
        PRIVATE
            -fvisibility=hidden
            -ferror-limit=5
    )

    # Generate coverage with Clang
    if(CMAKE_BUILD_TYPE MATCHES Debug)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage -g -O0")
        # propagate this coverage option
        target_compile_options(${PROJECT_NAME}
            PUBLIC
                -g -O0 --coverage
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
            # -fmodules-ts
    )
elseif(LINUX)
    target_include_directories(${PROJECT_NAME} 
        PUBLIC
            ${PROJECT_DIR}/prebuilt/include/c++/v1/
    )

    target_compile_options(${PROJECT_NAME} 
        PUBLIC
            -stdlib=libc++
            # For manual libc++ configuration
            -nostdinc++ -nodefaultlibs
    )
    
    target_link_libraries(${PROJECT_NAME}
        PUBLIC
            libc++abi.so
            libc++.so
            libc++experimental.a
    )
endif()
target_link_libraries(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_DL_LIBS}
)

# [cppmagic]: Installation

install(TARGETS ${PROJECT_NAME}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/libs/${CMAKE_BUILD_TYPE}/${PLATFORM}
)
install(
    FILES
        ${PROJECT_DIR}/magic/coroutine.hpp
        ${PROJECT_DIR}/magic/channel.hpp

        ${PROJECT_DIR}/magic/linkable.h
        ${PROJECT_DIR}/magic/switch.h
        ${PROJECT_DIR}/magic/sync.h
        
        # ${PROJECT_DIR}/magic/date_time.hpp
        # ${PROJECT_DIR}/magic/stop_watch.hpp
        # ${PROJECT_DIR}/magic/plugin.hpp
    DESTINATION 
        ${CMAKE_INSTALL_PREFIX}/include/magic/
)
if(LINUX)
    install(FILES ${PROJECT_DIR}/prebuilt/lib/libc++.so
        DESTINATION ${CMAKE_INSTALL_PREFIX}/libs
    )
endif()

# [cppmagic_test]: Test suite

if(ANDROID OR IOS)
    return()
endif()

set(TEST_NAME ${PROJECT_NAME}_test)

add_executable(${TEST_NAME}
    ${PROJECT_DIR}/test/test.entry.cpp
)
add_dependencies(${TEST_NAME} ${PROJECT_NAME})

if(NOT WIN32)
    target_compile_options(${TEST_NAME}
        PUBLIC
            -fvisibility=hidden
            -ferror-limit=5
    )
endif()
target_include_directories(${TEST_NAME}
    PRIVATE
        ${PROJECT_DIR}/test
)

target_link_libraries(${TEST_NAME}
    PUBLIC
        ${PROJECT_NAME}
)

if(UNIX AND NOT APPLE) 
    # target_compile_options(${TEST_NAME} 
    #     PRIVATE
    #         -nostdinc++ -nodefaultlibs
    # )
    # target_include_directories(${TEST_NAME} 
    #     PRIVATE
    #         ${PROJECT_DIR}/prebuilt/include/c++/v1/
    # )

    target_link_libraries(${PROJECT_NAME}
        PUBLIC
            libc++.so
            libc++experimental.a
    )
endif()
