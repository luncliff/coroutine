#
#   Author  : github.com/luncliff (luncliff@gmail.com)
#
cmake_minimum_required(VERSION 3.8)

add_executable(coroutine_test
    # files to support test code simplification
    test_shared.h
    test_runner.cpp test_catch2.cpp 

    # actual test codes
    test_coroutine_handle.cpp
    test_coro_return.cpp
    test_concrt_latch.cpp
    test_coro_yield_enumerable.cpp
    test_coro_yield_sequence.cpp
    test_coro_channel.cpp

    test_network.h
    test_network.cpp
    test_net_resolver.cpp
    test_net_echo_tcp.cpp
    # c2_socket_echo_udp.cpp
)
if(WIN32)
    target_sources(coroutine_test
    PRIVATE
        test_concrt_windows.cpp
    )
elseif(UNIX)
    target_sources(coroutine_test
    PRIVATE
        test_concrt_unix.cpp
    )
endif()

set_target_properties(coroutine_test
PROPERTIES
    CXX_STANDARD 17
)

target_include_directories(coroutine_test
PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/modules/
    ${GSL_INCLUDE_DIR}
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
        -g -Wall -Wextra -Wno-unknown-pragmas 
    )
endif()
