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
    # this cmake project is not for Windows/MSVC
    return(1) 
else()
    add_compile_options(-fPIC -Wall)
endif()

# Project Info

project(cppmagic LANGUAGES CXX 
        VERSION 1.0.2
        DESCRIPTION "Auto-magically working C++ coroutine"
)
message(STATUS "========== ${PROJECT_NAME} : ${PROJECT_VERSION} ==========")

# Alias for known variables
set(ROOT_DIR    ${CMAKE_SOURCE_DIR})
set(PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
message(STATUS "Root        \t: ${ROOT_DIR}")
message(STATUS "Project     \t: ${PROJECT_DIR}")

set(CMAKE_INSTALL_PREFIX ${PROJECT_DIR}/install)
message(STATUS "Install     \t: ${CMAKE_INSTALL_PREFIX}")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
message(STATUS "Build Type  \t: ${CMAKE_BUILD_TYPE}")

# System / Compiler

message(STATUS "System      \t: ${CMAKE_SYSTEM}")
message(STATUS "Compiler")
message(STATUS " - ID       \t: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS " - Path     \t: ${CMAKE_CXX_COMPILER}")

# [cppmagic]: Core features

add_library(${PROJECT_NAME}
    ${PROJECT_DIR}/msvc/dllmain.cpp
)
set_target_properties(${PROJECT_NAME} PROPERTIES 
    VERSION         ${PROJECT_VERSION}
    # CXX_STANDARD    17      # this property is equal to `-std=gnu++17`
    #                         # We will use `-std=c++1z` instead
)

target_include_directories(${PROJECT_NAME} 
    PUBLIC
        ${PROJECT_DIR}
    PRIVATE    
        ${PROJECT_DIR}/clang/
)
target_compile_options(${PROJECT_NAME} 
    PUBLIC
        -std=c++1z
        -fcoroutines-ts
        -fno-rtti
    PRIVATE
        -fvisibility=hidden
        -ferror-limit=5
)
if(APPLE)
    target_compile_options(${PROJECT_NAME} 
        PUBLIC
            -stdlib=libc++ 
        PRIVATE
            -fmodules-ts
    )
elseif(UNIX)  # Only for Unix/Linux
    target_include_directories(${PROJECT_NAME} 
        PRIVATE
            ${PROJECT_DIR}/prebuilt/include/c++/v1/
    )
    target_compile_options(${PROJECT_NAME} 
        PRIVATE
            # For manual libc++ configuration
            -nostdinc++ -nodefaultlibs
    )
    target_link_libraries(${PROJECT_NAME}
        PUBLIC
            ${PROJECT_DIR}/prebuilt/lib/libc++.so
            ${PROJECT_DIR}/prebuilt/lib/libc++experimental.a
    )
endif()

# [cppmagic]: Installation

install(TARGETS ${PROJECT_NAME}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/libs
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
if(UNIX AND NOT APPLE)
    install(FILES ${PROJECT_DIR}/prebuilt/lib/libc++.so
        DESTINATION ${CMAKE_INSTALL_PREFIX}/libs
    )
endif()

# [cppmagic_test]: Test suite for Non-Windows

set(TEST_NAME ${PROJECT_NAME}_test)

add_executable(${TEST_NAME}
    ${PROJECT_DIR}/test/test.entry.cpp
)
add_dependencies(${TEST_NAME} ${PROJECT_NAME})

target_compile_options(${TEST_NAME}
    PRIVATE
        -fvisibility=hidden
        -ferror-limit=5
)
target_include_directories(${TEST_NAME}
    PRIVATE
        ${PROJECT_DIR}/test
)

target_link_libraries(${TEST_NAME}
    PUBLIC
        ${PROJECT_NAME}
)

if(UNIX AND NOT APPLE) # Only for Unix/Linux
    target_compile_options(${TEST_NAME} 
        PRIVATE
            -nostdinc++ -nodefaultlibs
    )
    target_include_directories(${TEST_NAME} 
        PRIVATE
            ${PROJECT_DIR}/prebuilt/include/c++/v1/
    )
endif()
