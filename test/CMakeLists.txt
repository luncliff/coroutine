#
#   Author  : github.com/luncliff (luncliff@gmail.com)
#
cmake_minimum_required(VERSION 3.8)

add_library(helper_network STATIC
    test_network.h
    test_network.cpp
)
target_include_directories(helper_network
PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${GSL_INCLUDE_DIR}
)
target_link_libraries(helper_network
PUBLIC
    coroutine
)

# generate_exe_test( ... )
function(generate_exe_test NAME)
    set(TEST_NAME test_${NAME})
    # create a test exe with the given name ...
    add_executable(${TEST_NAME} ${NAME}.cpp)
    add_test(${NAME} ${TEST_NAME})

    # ... example cmake configuration of the library user ...
    set_target_properties(${TEST_NAME}
    PROPERTIES
        CXX_STANDARD 17
    )
    target_include_directories(${TEST_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${GSL_INCLUDE_DIR}
    )
    target_link_libraries(${TEST_NAME}
    PUBLIC
        coroutine ${ARGV1}
    )
    target_compile_definitions(${TEST_NAME}
    PRIVATE
        CMAKE_TEST
    )
    if(WIN32)
        target_compile_definitions(${TEST_NAME}
        PRIVATE
            _CRT_SECURE_NO_WARNINGS
        )
        target_compile_options(${TEST_NAME}
        PRIVATE
            /Od # temporary disable optimization. 
                # will be removed after test code changes
        )
    elseif(UNIX)
        # code coverage option lead to compiler crash
        # list(APPEND CMAKE_CXX_FLAGS "--coverage")
        target_compile_options(${TEST_NAME}
        PRIVATE
            -O0 # temporary disable optimization. 
                # will be removed after test code changes
            -g -Wall -Wextra -Wno-unknown-pragmas 
        )
    endif()
endfunction()

generate_exe_test( enumerable_accumulate )
generate_exe_test( enumerable_iterator )
generate_exe_test( enumerable_max_element )
generate_exe_test( enumerable_move )
generate_exe_test( enumerable_yield_never )
generate_exe_test( enumerable_yield_once )

generate_exe_test( sequence_destroy )
generate_exe_test( sequence_frame_status )
generate_exe_test( sequence_suspend_await )
generate_exe_test( sequence_suspend_yield )
generate_exe_test( sequence_yield_never )
generate_exe_test( sequence_yield_once )

generate_exe_test( handle_swap )
generate_exe_test( handle_move )

generate_exe_test( latch_wait_after_ready )
generate_exe_test( latch_wait_multiple_times )

generate_exe_test( coro_no_return )
generate_exe_test( coro_frame_awaitable )
generate_exe_test( coro_frame_empty )
generate_exe_test( coro_frame_return )
generate_exe_test( coro_frame_suspend )

if(WIN32)

generate_exe_test( win32_get_threads )

generate_exe_test( event_win_cancel )
generate_exe_test( event_win_signal )
generate_exe_test( event_win_wait_one )
generate_exe_test( event_win_wait_array )

elseif(UNIX)

generate_exe_test( event_unix_no_wait )
generate_exe_test( event_unix_signal_multiple_times )
generate_exe_test( event_unix_ready_after_signaled )
generate_exe_test( event_unix_wait_multiple_times )
generate_exe_test( event_unix_wait_one )
generate_exe_test( event_unix_wait_three )

endif()

generate_exe_test( net_get_addrinfo_ip6_bind            helper_network )
generate_exe_test( net_get_addrinfo_ip6_multicast       helper_network )
generate_exe_test( net_get_addrinfo_tcp6_connect        helper_network )
generate_exe_test( net_get_addrinfo_tcp6_listen_numeric helper_network )
generate_exe_test( net_get_addrinfo_tcp6_listen_text    helper_network )
generate_exe_test( net_get_addrinfo_udp6_unspecified    helper_network )
generate_exe_test( net_get_addrinfo_udp6_v4mapped       helper_network )

generate_exe_test( net_get_nameinfo_v4  helper_network )
generate_exe_test( net_get_nameinfo_v6  helper_network )

generate_exe_test( net_get_hostname     helper_network )

generate_exe_test( net_echo_tcp         helper_network )
generate_exe_test( net_echo_udp         helper_network )
