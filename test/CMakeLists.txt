# ---------------------------------------------------------------------------
#
#   Author  : github.com/luncliff (luncliff@gmail.com)
#
# ---------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.8)

# if package does not exist, use submodule instead
find_package(Catch2)
if(NOT Catch2_FOUND)
    add_subdirectory(${PROJECT_SOURCE_DIR}/external/catch2
                     ${CMAKE_BINARY_DIR}/catch2
    )
endif()

add_executable(coroutine_test
    catch2_entry.cpp

    net/catch2_resolver.cpp
    net/socket_test.h
    net/catch2_socket.cpp
    net/catch2_socket_echo_udp.cpp
    net/catch2_socket_echo_tcp.cpp
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

if(WIN32)
    target_compile_definitions(coroutine_test
    PRIVATE
        _CRT_SECURE_NO_WARNINGS
    )
elseif(UNIX)
    # code coverage option lead to compiler crash
    # list(APPEND CMAKE_CXX_FLAGS "--coverage")
    target_link_libraries(coroutine_test
    PRIVATE
        stdc++ # solve possible gnu-dependency
    )
    target_compile_options(coroutine_test
    PRIVATE
        -std=c++2a -stdlib=libc++
        -g
        -Wall -Wextra
        -Wno-unknown-pragmas # ignore pragma incompatibility
    )
endif()
