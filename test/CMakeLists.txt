cmake_minimum_required(VERSION 3.12)

# create_ctest( ... )
function(create_ctest TEST_NAME)
    # create a test exe with the given name ...
    add_executable(${TEST_NAME} ${TEST_NAME}.cpp)
    add_test(NAME ${TEST_NAME} 
             COMMAND ${TEST_NAME})

    set_target_properties(${TEST_NAME}
    PROPERTIES
        CXX_STANDARD 20
    )
    target_include_directories(${TEST_NAME}
    PRIVATE
        ${GSL_INCLUDE_DIR}
    )
    # all arguments after TEST_NAME 
    # should be library (or CMake target) name
    foreach(idx RANGE 1 ${ARGC})
        target_link_libraries(${TEST_NAME}
        PRIVATE
            ${ARGV${idx}} 
        )
    endforeach()

    target_compile_definitions(${TEST_NAME}
    PRIVATE
        CMAKE_TEST _DEBUG
    )
    if(WIN32)
    elseif(APPLE)
        target_link_libraries(${TEST_NAME}
        PRIVATE
            c++
        )
    else()
        target_link_libraries(${TEST_NAME}
        PRIVATE
            stdc++ c++
        )
    endif()
endfunction()

create_ctest( article_russian_roulette  coroutine_portable )

#
#   <internal/yield.hpp>
#
create_ctest( enumerable_accumulate     coroutine_portable )
create_ctest( enumerable_iterator       coroutine_portable )
create_ctest( enumerable_max_element    coroutine_portable )
create_ctest( enumerable_move           coroutine_portable )
create_ctest( enumerable_yield_never    coroutine_portable )
create_ctest( enumerable_yield_once     coroutine_portable )
create_ctest( enumerable_yield_rvalue   coroutine_portable )

#
#   <coroutine/frame.h>
#   <coroutine/return.h>
#
create_ctest( return_destroy_with_handle  coroutine_portable )
create_ctest( return_destroy_with_return  coroutine_portable )
create_ctest( return_not_coroutine        coroutine_portable )
create_ctest( return_not_subroutine       coroutine_portable )

#
#   <coroutine/net.h>
#
if(TARGET coroutine_net)
create_ctest( net_resolve_ip4   coroutine_net ssf )
create_ctest( net_resolve_ip6   coroutine_net ssf )
# create_ctest( net_socket_tcp_echo coroutine_net )
# create_ctest( net_socket_udp_echo coroutine_net )
create_ctest( net_resolve_ip6_bind             coroutine_net ssf )
create_ctest( net_resolve_ip6_multicast        coroutine_net ssf )
create_ctest( net_resolve_tcp6_connect         coroutine_net ssf )
create_ctest( net_resolve_tcp6_listen_text     coroutine_net ssf )
create_ctest( net_resolve_tcp6_listen_numeric  coroutine_net ssf )
create_ctest( net_resolve_udp6_unspecified     coroutine_net ssf )
create_ctest( net_resolve_udp6_v4mapped        coroutine_net ssf )
endif()

#
#   <coroutine/windows.h>
#   <coroutine/unix.h>, <coroutine/linux.h>
#   <coroutine/pthread.h>
#
if(TARGET coroutine_system)
if(WIN32)
create_ctest( windows_event_set             coroutine_system )
create_ctest( windows_event_cancel          coroutine_system )
create_ctest( windows_event_wait_one        coroutine_system )
create_ctest( windows_event_wait_multiple   coroutine_system )
create_ctest( windows_on_apc_known          coroutine_system )
create_ctest( windows_on_apc_self           coroutine_system )
create_ctest( windows_on_thread_pool        coroutine_system )

elseif(UNIX)
create_ctest( pthread_detach_no_spawn       coroutine_system )
# create_ctest( pthread_detach_spawn          coroutine_system )
create_ctest( pthread_join_no_spawn         coroutine_system )
create_ctest( pthread_join_spawn_1          coroutine_system )
create_ctest( pthread_join_spawn_2          coroutine_system )

endif()

if(CMAKE_SYSTEM_NAME MATCHES Linux)
create_ctest( linux_event_no_wait       coroutine_system )
create_ctest( linux_event_wait          coroutine_system )
create_ctest( linux_event_signal        coroutine_system )

elseif(UNIX)
create_ctest( unix_kqueue_single_thread  coroutine_system )

endif()
endif()


#
#   <internal/channel.hpp>
#
create_ctest( channel_close_read            coroutine_system )
create_ctest( channel_close_write           coroutine_system )
create_ctest( channel_ownership_consumer    coroutine_system )
create_ctest( channel_ownership_producer    coroutine_system )
create_ctest( channel_read_write_mutex      coroutine_system )
create_ctest( channel_read_write_nolock     coroutine_system )
create_ctest( channel_write_read_mutex      coroutine_system )
create_ctest( channel_write_read_nolock     coroutine_system )
create_ctest( channel_select_empty          coroutine_system )
create_ctest( channel_select_type           coroutine_system )
if(WIN32)
create_ctest( channel_race_condition        coroutine_system latch )
endif()
