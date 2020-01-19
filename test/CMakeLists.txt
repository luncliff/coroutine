cmake_minimum_required(VERSION 3.12)

# create_ctest( ... )
function(create_ctest FILE_NAME LIB_NAME)
    set(TEST_NAME test_${FILE_NAME})

    # create a test exe with the given name ...
    add_executable(${TEST_NAME} ${FILE_NAME}.cpp)
    add_test(${FILE_NAME} ${TEST_NAME})
    
    set_target_properties(${TEST_NAME}
    PROPERTIES
        CXX_STANDARD 20
    )
    target_include_directories(${TEST_NAME}
    PRIVATE
        ${GSL_INCLUDE_DIR}
    )
    target_link_libraries(${TEST_NAME}
    PRIVATE
        ${LIB_NAME}
    )
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
create_ctest( net_get_name_ip4 coroutine_net )
create_ctest( net_get_name_ip6 coroutine_net )
# create_ctest( net_socket_tcp_echo coroutine_net )
# create_ctest( net_socket_udp_echo coroutine_net )
create_ctest( net_get_address_ip6_bind             coroutine_net )
create_ctest( net_get_address_ip6_multicast        coroutine_net )
create_ctest( net_get_address_tcp6_connect         coroutine_net )
create_ctest( net_get_address_tcp6_listen_text     coroutine_net )
create_ctest( net_get_address_tcp6_listen_numeric  coroutine_net )
create_ctest( net_get_address_udp6_unspecified     coroutine_net )
create_ctest( net_get_address_udp6_v4mapped        coroutine_net )
