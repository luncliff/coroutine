# ---------------------------------------------------------------------------
#
#   Author  : github.com/luncliff (luncliff@gmail.com)
#
# ---------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.8)

# if package does not exist, use submodule instead
find_package(Catch2)
if(NOT Catch2_FOUND)    
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../external/catch2
                     ${CMAKE_BINARY_DIR}/catch2
    )
endif()

# testing the library 
add_executable(coroutine_test
    scenario/entry.cpp
    scenario/switch_to.cpp
    scenario/channel.cpp
    case/message.cpp
    case/sync.cpp
    case/returns.cpp
    case/generator.cpp
    case/async_generator.cpp
)

set_target_properties(coroutine_test
PROPERTIES
    CXX_STANDARD 17
)

target_include_directories(coroutine_test
PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/modules/
)

target_link_libraries(coroutine_test
PUBLIC
    coroutine
    Catch2::Catch2
)

if(UNIX)
    # code coverage option lead to compiler crash
    # list(APPEND CMAKE_CXX_FLAGS "--coverage")
    target_link_libraries(coroutine_test
    PRIVATE
        stdc++ # solve possible gnu-dependency
    )
    target_compile_options(coroutine_test
    PRIVATE
        -std=c++2a -stdlib=libc++ -g
    )
endif()
