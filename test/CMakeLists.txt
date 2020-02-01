cmake_minimum_required(VERSION 3.12)

# create_ctest( ... )
function(create_ctest TEST_NAME)
    # create a test exe with the given name ...
    add_executable(${TEST_NAME} ${TEST_NAME}.cpp)
    add_test(${TEST_NAME} ${TEST_NAME})
    
    set_target_properties(${TEST_NAME}
    PROPERTIES
        CXX_STANDARD 20
    )
    target_include_directories(${TEST_NAME}
    PRIVATE
        ${GSL_INCLUDE_DIR}
    )
    foreach(idx RANGE 1 ${ARGC})
        target_link_libraries(${TEST_NAME}
        PRIVATE
            ${ARGV${idx}}
        )
    endforeach()

    target_compile_definitions(${TEST_NAME}
    PRIVATE
        CMAKE_TEST
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
create_ctest( return_frame_destroy_with_handle  coroutine_portable )
create_ctest( return_frame_destroy_with_return  coroutine_portable )
create_ctest( return_frame_not_coroutine        coroutine_portable )
create_ctest( return_frame_not_subroutine       coroutine_portable )

#
#   <coroutine/net.h>
#
if(TARGET coroutine_net)
create_ctest( net_get_name_ip4 coroutine_net ssf )
create_ctest( net_get_name_ip6 coroutine_net ssf )
# create_ctest( net_socket_tcp_echo coroutine_net )
# create_ctest( net_socket_udp_echo coroutine_net )
create_ctest( net_get_address_ip6_bind             coroutine_net ssf )
create_ctest( net_get_address_ip6_multicast        coroutine_net ssf )
create_ctest( net_get_address_tcp6_connect         coroutine_net ssf )
create_ctest( net_get_address_tcp6_listen_text     coroutine_net ssf )
create_ctest( net_get_address_tcp6_listen_numeric  coroutine_net ssf )
create_ctest( net_get_address_udp6_unspecified     coroutine_net ssf )
create_ctest( net_get_address_udp6_v4mapped        coroutine_net ssf )
endif()

#
#   <coroutine/windows.h>
#
if(TARGET coroutine_system)
if(WIN32)
create_ctest( windows_event_set              coroutine_system )
create_ctest( windows_event_cancel           coroutine_system )
create_ctest( windows_event_wait_one         coroutine_system )
create_ctest( windows_event_wait_multiple    coroutine_system )
create_ctest( windows_on_apc_known           coroutine_system )
create_ctest( windows_on_apc_self            coroutine_system )
create_ctest( windows_on_thread_pool         coroutine_system )
endif()
if(UNIX)
create_ctest( unix_kqueue_single_thread  coroutine_system )
endif()
if(LINUX)
endif()
endif()
