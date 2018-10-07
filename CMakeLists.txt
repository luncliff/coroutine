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

# Variable setup
if(WIN32 AND MSVC)
    message(FATAL_ERROR "This cmake project is not for Windows/MSVC")
    return(1) 
endif()

project(cppmagic LANGUAGES CXX VERSION 1.0.4)

set(ROOT_DIR    ${CMAKE_SOURCE_DIR})
set(PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
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

if(NOT IOS)
    SET(BUILD_SHARED_LIBS true)
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

if(NOT IOS AND NOT WIN32)
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
    ${PROJECT_DIR}/src/dllmain.cpp
)

set_target_properties(${PROJECT_NAME} PROPERTIES 
    VERSION         ${PROJECT_VERSION}
    # CXX_STANDARD    17      # this property leads `-std=gnu++17`
    #                         # We will use `-std=c++1z` instead
)

target_include_directories(${PROJECT_NAME} 
    PUBLIC
        ${PROJECT_DIR}/src/
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_DL_LIBS}
)

if(WIN32)  # Clang + Windows

    # target_sources(${PROJECT_NAME}    
    #     PRIVATE
    #         ${PROJECT_DIR}/magic/switch.h
    # )

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
        PRIVATE
            -Wno-unused-function
            -Wno-c++98-compat 
            -Wno-reserved-id-macro 
            -Wno-missing-prototypes
    )
    # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Xclang -fcoroutines-ts")

    set_target_properties(${PROJECT_NAME} PROPERTIES 
        LINK_FLAGS "${LINK_FLAGS} /errorReport:send")

else() # Clang + Non-Windows
    target_compile_options(${PROJECT_NAME} 
        PUBLIC
            -std=c++1z
            -fcoroutines-ts
            -fPIC 
        PRIVATE
            -Wall
            -fno-rtti 
            -fvisibility=hidden
            -ferror-limit=5
    )

    if(CMAKE_BUILD_TYPE MATCHES Debug)
        # Generate coverage with Clang
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
            # -fmodules-ts
    )
elseif(LINUX)
    target_include_directories(${PROJECT_NAME} 
        PUBLIC
            ${PROJECT_DIR}/prebuilt/include/c++/v1/
    )

    target_compile_options(${PROJECT_NAME} 
        PUBLIC
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

    # <asm/types.h>
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
    
    # target_link_libraries(${PROJECT_NAME}
    #     PUBLIC
    #         android
    # )

endif() # Android

# [cppmagic]: Installation

install(TARGETS ${PROJECT_NAME}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/libs/${CMAKE_BUILD_TYPE}/${PLATFORM}
)
install(
    FILES
        ${PROJECT_DIR}/src/magic/coroutine.hpp
        ${PROJECT_DIR}/src/magic/channel.hpp

        ${PROJECT_DIR}/src/magic/linkable.h
        ${PROJECT_DIR}/src/magic/switch.h
        ${PROJECT_DIR}/src/magic/sync.h
        
        # ${PROJECT_DIR}/magic/date_time.hpp
        # ${PROJECT_DIR}/magic/stop_watch.hpp
        # ${PROJECT_DIR}/magic/plugin.hpp
    DESTINATION 
        ${CMAKE_INSTALL_PREFIX}/include/magic/
)
if(LINUX)
    install(FILES ${PROJECT_DIR}/prebuilt/lib/libc++experimental.a
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
else()
    target_compile_options(${TEST_NAME}
    PRIVATE
        -Wno-c++98-compat 
        -Wno-reserved-id-macro 
        -Wno-missing-prototypes
    )
endif()

target_include_directories(${TEST_NAME}
    PRIVATE
        ${PROJECT_DIR}/test
)

target_link_libraries(${TEST_NAME}
    PUBLIC
        ${CMAKE_DL_LIBS}
        ${PROJECT_NAME}
)
