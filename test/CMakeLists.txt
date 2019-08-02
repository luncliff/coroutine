#
#   Author  : github.com/luncliff (luncliff@gmail.com)
#
cmake_minimum_required(VERSION 3.8)

# generate_exe_test( ... )
function(generate_exe_test NAME)
    set(TEST_NAME test_${NAME})
    # create a test exe with the given name ...
    add_executable(${TEST_NAME}
        ${NAME}.cpp
        test.h test.cpp # test adapters
    )
    add_test(${NAME} ${TEST_NAME})

    # ... example cmake configuration of the library user ...
    set_target_properties(${TEST_NAME}
    PROPERTIES
        CXX_STANDARD 17
    )
    target_include_directories(${TEST_NAME}
    PRIVATE
        ${BUILD_INTERFACE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${GSL_INCLUDE_DIR}
    )
    target_link_libraries(${TEST_NAME}
    PUBLIC
        coro::portable coro::event coro::thread
        concrt ${ARGV1}
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
    elseif(UNIX)
        # if(${CMAKE_CXX_COMPILER_ID} MATCHES Clang)
        #     target_compile_options(${TEST_NAME}
        #     PUBLIC
        #         -std=c++2a -stdlib=libc++ -fcoroutines-ts 
        #         -fPIC
        #     PRIVATE
        #         -Wall -Wno-unknown-pragmas -Wno-unused-private-field
        #         -fno-rtti -fvisibility=hidden -ferror-limit=5
        #     )
        # elseif(${CMAKE_CXX_COMPILER_ID} MATCHES GNU)
        #     target_compile_options(${TEST_NAME}
        #     PUBLIC
        #         -std=gnu++2a -fcoroutines
        #         -fPIC
        #         -fno-exceptions
        #         -fconcepts
        #     PRIVATE
        #         -Wall -Wno-unknown-pragmas
        #         -fno-rtti -fvisibility=hidden
        #     )
        #     target_link_libraries(${TEST_NAME}
        #     PUBLIC
        #         stdc++
        #     )
        # endif()
    endif()
endfunction()

if(${CMAKE_CXX_COMPILER_ID} MATCHES Clang)

generate_exe_test( clang_builtin_intrinsics )

elseif(${CMAKE_CXX_COMPILER_ID} MATCHES GNU)

generate_exe_test( gcc_support_version )
generate_exe_test( gcc_concepts_promise )
generate_exe_test( gcc_concepts_awaitable )

endif()



#
#   <coroutine/frame.h>
#

# generate_exe_test( handle_swap )
# generate_exe_test( handle_move )

#
#   <coroutine/return.h>
#

generate_exe_test( return_forget_frame_with_await )
generate_exe_test( return_forget_frame_subroutine )
generate_exe_test( return_preserve_frame_empty )
generate_exe_test( return_preserve_frame_destroy_with_handle )
generate_exe_test( return_preserve_frame_destroy_with_return )

generate_exe_test( yield_enumerable_accumulate )
generate_exe_test( yield_enumerable_iterator )
generate_exe_test( yield_enumerable_max_element )
generate_exe_test( yield_enumerable_move )
generate_exe_test( yield_enumerable_yield_never )
generate_exe_test( yield_enumerable_yield_once )

# generate_exe_test( yield_sequence_destroy )
# generate_exe_test( yield_sequence_frame_status )
# generate_exe_test( yield_sequence_suspend_await )
# generate_exe_test( yield_sequence_suspend_yield )
# generate_exe_test( yield_sequence_yield_never )
# generate_exe_test( yield_sequence_yield_once )

generate_exe_test( channel_read_fail_after_close )
generate_exe_test( channel_read_write_mutex )
generate_exe_test( channel_read_write_nolock )
generate_exe_test( channel_write_fail_after_close )
generate_exe_test( channel_write_read_mutex )
generate_exe_test( channel_write_read_nolock )
generate_exe_test( channel_select_bypass_empty )
generate_exe_test( channel_select_type_match )
generate_exe_test( channel_select_peek_all_cases )
generate_exe_test( channel_race_no_leak )
generate_exe_test( channel_ownership_consumer )
generate_exe_test( channel_ownership_producer )

if(WIN32)

# generate_exe_test( concrt_win32_get_threads )
# generate_exe_test( event_win_cancel )
# generate_exe_test( event_win_signal )
# generate_exe_test( event_win_wait_one )
# generate_exe_test( event_win_wait_array )

# generate_exe_test( latch_wait_after_ready )
# generate_exe_test( latch_wait_multiple_times )

elseif(UNIX)

# generate_exe_test( concrt_pthread_life )

generate_exe_test( event_unix_no_wait )
generate_exe_test( event_unix_signal_multiple_times )
generate_exe_test( event_unix_ready_after_signaled )
generate_exe_test( event_unix_wait_multiple_times )
generate_exe_test( event_unix_wait_one )
generate_exe_test( event_unix_wait_three )

# generate_exe_test( pthread_joiner_multiple_spawn )
# generate_exe_test( pthread_joiner_no_spawn )


endif()

add_library(socket STATIC
    socket.h
    socket.cpp
)
target_include_directories(socket
PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(socket
PUBLIC
    coro::net
)

generate_exe_test( net_get_addrinfo_ip6_bind            socket )
generate_exe_test( net_get_addrinfo_ip6_multicast       socket )
generate_exe_test( net_get_addrinfo_tcp6_connect        socket )
generate_exe_test( net_get_addrinfo_tcp6_listen_numeric socket )
generate_exe_test( net_get_addrinfo_tcp6_listen_text    socket )
generate_exe_test( net_get_addrinfo_udp6_unspecified    socket )
generate_exe_test( net_get_addrinfo_udp6_v4mapped       socket )
generate_exe_test( net_get_nameinfo_v4  socket )
generate_exe_test( net_get_nameinfo_v6  socket )
# generate_exe_test( net_get_hostname     socket )
# generate_exe_test( net_echo_tcp         socket )
# generate_exe_test( net_echo_udp         socket )
