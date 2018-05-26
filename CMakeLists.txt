# CMake for non-windows project

cmake_minimum_required(VERSION 3.6)

# Global setup

if(WIN32 OR MSVC)
    return(1) # this cmake project is not for Windows/MSVC
else()
    add_compile_options(-fPIC -Wall)
endif()

# Project Info

project(magic LANGUAGES CXX 
        VERSION 1.0.1
        DESCRIPTION "Auto-magically working C++ coroutine"
)

message(STATUS "========== ${PROJECT_NAME} : ${PROJECT_VERSION} ==========")

set(ROOT_DIR    ${CMAKE_SOURCE_DIR})
set(PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
message(STATUS "Root      \t: ${ROOT_DIR}")
message(STATUS "Project   \t: ${PROJECT_DIR}")

set(CMAKE_INSTALL_PREFIX ${PROJECT_DIR}/install)
message(STATUS "Install   \t: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "System    \t: ${CMAKE_SYSTEM}")

# Compiler/Toolchain

message(STATUS "Compiler")
message(STATUS " - ID     \t: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS " - Path   \t: ${CMAKE_CXX_COMPILER}")


add_library(${PROJECT_NAME} SHARED
    ${PROJECT_DIR}/msvc/dllmain.cpp
)

target_include_directories(${PROJECT_NAME} 
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE    
        ${PROJECT_DIR}/clang/
        ${PROJECT_DIR}/libcxx/include/
)

set_target_properties(${PROJECT_NAME} PROPERTIES 
    VERSION ${PROJECT_VERSION}
)

# set_target_properties(${PROJECT_NAME} PROPERTIES 
#     CXX_STANDARD 17
# )
target_compile_options(${PROJECT_NAME} 
    PUBLIC
        -std=c++2a -fcoroutines-ts -fmodules-ts
    PRIVATE
        -fvisibility=hidden
)
if(APPLE)
    target_compile_options(${PROJECT_NAME} 
        PUBLIC 
            -stdlib=libc++
    )
elseif(UNIX)
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
            libcxx
    )
endif()

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        c++experimental
)

install(TARGETS ${PROJECT_NAME}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/libs
)
install(FILES ${PROJECT_DIR}/magic/coroutine.hpp
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/magic/
)

# Test suite for Non-Windows

set(TEST_NAME ${PROJECT_NAME}_test)

add_executable(${TEST_NAME}
    ${PROJECT_DIR}/test/test.entry.cpp
)

target_include_directories(${TEST_NAME}
    PRIVATE
        ${PROJECT_DIR}/test
)
target_compile_options(${TEST_NAME}
    PUBLIC
        -std=c++2a -stdlib=libc++
    PRIVATE
        -fvisibility=hidden
)
target_link_libraries(${TEST_NAME}
    PUBLIC
        ${PROJECT_NAME}
)