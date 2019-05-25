#
#   Author  : github.com/luncliff (luncliff@gmail.com)
#
cmake_minimum_required(VERSION 3.8)

# function(generate_exe_test TEST_NAME)
#     # create a test exe with the given name ...
#     add_executable(${TEST_NAME}
#         test_shared.h test_catch2.cpp
#         ${TEST_NAME}.cpp
#     )
#     add_test(${TEST_NAME} ${TEST_NAME})
# endfunction()
# generate_exe_test( ... )

add_executable(coroutine_test
    # files to support test code simplification
    test_shared.h test_catch2.cpp
    test_network.h  test_network.cpp
    
    # actual test codes
    test_coroutine_handle.cpp
    test_coro_return.cpp
    test_concrt_latch.cpp
    test_coro_yield_enumerable.cpp
    test_coro_yield_sequence.cpp
    test_coro_channel.cpp
    test_net_resolver.cpp
    test_net_echo_tcp.cpp
    test_net_echo_udp.cpp
)
# add_test(coroutine_handle   coroutine_test [primitive]  )
# add_test(async_generator    coroutine_test [async]      )

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

# cmake configuration of the user's library project ...
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
    target_compile_options(coroutine_test
    PRIVATE
        -g -Wall -Wextra -Wno-unknown-pragmas 
    )
endif()
