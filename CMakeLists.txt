# CMake for non-windows project

cmake_minimum_required(VERSION 3.6)

# Global setup

if(WIN32 OR MSVC)
    return(1) # this cmake project is not for Windows/MSVC
else()
    add_compile_options(-fPIC -Wall)
endif()

# Project Info

project(cppmagic LANGUAGES CXX 
        VERSION 1.0.1
        # DESCRIPTION "Auto-magically working C++ coroutine"
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

# [cppmagic]: Core features

add_library(${PROJECT_NAME}
    ${PROJECT_DIR}/msvc/dllmain.cpp
)
set_target_properties(${PROJECT_NAME} PROPERTIES 
    VERSION         ${PROJECT_VERSION}
    # CXX_STANDARD    17      # this property is equal to `-std=gnu++17`
    #                         # We will use `-std=c++1z`
)

target_include_directories(${PROJECT_NAME} 
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE    
        ${PROJECT_DIR}/clang/
)
target_compile_options(${PROJECT_NAME} 
    PUBLIC
        -std=c++1z
        -fcoroutines-ts
    PRIVATE
        # -nostdinc++     # For manual libc++ configuration
        -fvisibility=hidden
)
if(APPLE)
    target_compile_options(${PROJECT_NAME} 
        PUBLIC
            -stdlib=libc++ 
        PRIVATE
            -fmodules-ts
    )
elseif(UNIX) #  AND NOT APPLE
    target_include_directories(${PROJECT_NAME} 
        PRIVATE
            ${PROJECT_DIR}/libcpp/include/c++/v1/
    )
    # target_compile_options(${PROJECT_NAME} 
    #     PUBLIC
    #         -nostdinc++  -nodefaultlibs
    #         # -I${PROJECT_DIR}/libcpp/include/c++/v1/
    #         # -l${PROJECT_DIR}/libcpp/lib/libc++.so
    # )
    # target_link_libraries(${PROJECT_NAME}
    #     PRIVATE
    #         c++
    # )
endif()

# target_link_libraries(${PROJECT_NAME}
#     PUBLIC
#         c++experimental
# )

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

# [cppmagic_test]: Test suite for Non-Windows

set(TEST_NAME ${PROJECT_NAME}_test)

add_executable(${TEST_NAME}
    ${PROJECT_DIR}/test/test.entry.cpp
)
add_dependencies(${TEST_NAME} ${PROJECT_NAME})

target_compile_options(${TEST_NAME}
    PRIVATE
        -fvisibility=hidden
        -ferror-limit=1
)
target_include_directories(${TEST_NAME}
    PRIVATE
        ${PROJECT_DIR}/test
)

target_link_libraries(${TEST_NAME}
    PUBLIC
        ${PROJECT_NAME}
)

if(APPLE)
    # nothing for apple
elseif(UNIX) #  AND NOT APPLE
    target_include_directories(${TEST_NAME} 
        PRIVATE
            ${PROJECT_DIR}/libcpp/include/c++/v1/
    )
    # target_compile_options(${TEST_NAME} 
    #     PRIVATE
    #         # -I${PROJECT_DIR}/libcpp/include/c++/v1/
    #         # -l${PROJECT_DIR}/libcpp/lib/libc++.so
    # )
    # target_link_libraries(${TEST_NAME}
    #     PUBLIC
    #         stdc++
    # )
endif()
