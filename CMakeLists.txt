cmake_minimum_required(VERSION 3.20)
project(coroutine LANGUAGES CXX VERSION 2.0.0)

option(BUILD_SHARED_LIBS    "https://cmake.org/cmake/help/latest/variable/BUILD_SHARED_LIBS.html" ON)
option(CMAKE_BUILD_TYPE     "https://cmake.org/cmake/help/latest/variable/CMAKE_BUILD_TYPE.html"  "Debug")
option(BUILD_TESTING        "Build test program" ON) # include(CTest)

set(CMAKE_CXX_STANDARD 17)

message(STATUS "system: ${CMAKE_SYSTEM}")
message(STATUS "build_type: ${CMAKE_BUILD_TYPE}")
message(STATUS "paths:")
message(STATUS " - ${PROJECT_SOURCE_DIR}")
message(STATUS " - ${CMAKE_INSTALL_PREFIX}")
message(STATUS " - ${CMAKE_SOURCE_DIR}")
message(STATUS " - ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS)
message(STATUS "compiler:")
message(STATUS " - ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS " - ${CMAKE_CXX_COMPILER}")
message(STATUS)
message(STATUS "cmake:")
message(STATUS " - ${CMAKE_VERSION}")
message(STATUS " - ${CMAKE_COMMAND}")
message(STATUS " - ${CMAKE_TOOLCHAIN_FILE}")
message(STATUS " - ${CMAKE_GENERATOR}")
message(STATUS " - ${CMAKE_BUILD_TOOL}")
message(STATUS)

# get_filename_component(CMAKE_COROUTINES_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/FindCoroutines.cmake ABSOLUTE)
# if(NOT EXISTS ${CMAKE_COROUTINES_MODULE_PATH})
#     file(DOWNLOAD https://raw.githubusercontent.com/facebookexperimental/libunifex/main/cmake/FindCoroutines.cmake
#                 ${CMAKE_COROUTINES_MODULE_PATH}
#         EXPECTED_HASH SHA256=0129305dd3d030de21684543fec9b1e7b0af7a67ed1bad4348c3259ecfb20cef
#     )
# endif()

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

#
# Known STL-Compiler issues
#   - https://github.com/microsoft/STL, issue 100
#
if(CMAKE_CXX_COMPILER_ID MATCHES Clang AND WIN32)
    message(WARNING "clang-cl won't work with <experimental/coroutine>")
endif()
find_package(Coroutines REQUIRED)

find_package(Microsoft.GSL CONFIG)
if(Microsoft.GSL_FOUND)
    get_target_property(GSL_INCLUDE_DIR Microsoft.GSL::GSL INTERFACE_INCLUDE_DIRECTORIES)
else()
    find_path(GSL_INCLUDE_DIR "gsl/gsl" REQUIRED)
    # add_subdirectory(external/ms-gsl)
    # get_filename_component(GSL_INCLUDE_DIR 
    #                        ${PROJECT_SOURCE_DIR}/external/ms-gsl/include ABSOLUTE)
endif()

return()

set(MODULE_INTERFACE_DIR ${PROJECT_SOURCE_DIR}/interface)
add_subdirectory(modules/portable)	# corotuien_portable
add_subdirectory(modules/system)    # coroutine_system
add_subdirectory(modules/net)       # coroutine_net

install(FILES           ${MODULE_INTERFACE_DIR}/coroutine/frame.h
                        ${MODULE_INTERFACE_DIR}/coroutine/return.h
                        ${MODULE_INTERFACE_DIR}/coroutine/channel.hpp
                        ${MODULE_INTERFACE_DIR}/coroutine/yield.hpp
        DESTINATION     ${CMAKE_INSTALL_PREFIX}/include/coroutine
)
if(WIN32)
    install(FILES       ${MODULE_INTERFACE_DIR}/coroutine/windows.h
            DESTINATION ${CMAKE_INSTALL_PREFIX}/include/coroutine
    )

elseif(CMAKE_SYSTEM_NAME MATCHES Linux)
    install(FILES       ${MODULE_INTERFACE_DIR}/coroutine/linux.h
                        ${MODULE_INTERFACE_DIR}/coroutine/pthread.h
            DESTINATION ${CMAKE_INSTALL_PREFIX}/include/coroutine
    )

elseif(UNIX OR APPLE)
    install(FILES       ${MODULE_INTERFACE_DIR}/coroutine/unix.h
                        ${MODULE_INTERFACE_DIR}/coroutine/pthread.h
            DESTINATION ${CMAKE_INSTALL_PREFIX}/include/coroutine
    )

endif()
install(FILES			${MODULE_INTERFACE_DIR}/coroutine/net.h
        DESTINATION     ${CMAKE_INSTALL_PREFIX}/include/coroutine
)

install(TARGETS     coroutine_portable coroutine_system coroutine_net
        EXPORT      coroutine-config
        INCLUDES    DESTINATION     ${CMAKE_INSTALL_PREFIX}/include
        RUNTIME     DESTINATION     ${CMAKE_INSTALL_PREFIX}/bin
        LIBRARY     DESTINATION     ${CMAKE_INSTALL_PREFIX}/lib
        ARCHIVE     DESTINATION     ${CMAKE_INSTALL_PREFIX}/lib
)

#
# export declared cmake targets
#
# 'coroutine-targets' is indeed better name, but without using 'configure_file()'
# the exporting step will be more complicated for non-CMake users.
# just merge all contents into the file 'coroutine-config.cmake'
#
install(EXPORT      ${PROJECT_NAME}-config
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}
)

#
# generate/install config & version info
#
include(CMakePackageConfigHelpers)
set(VERSION_FILE_PATH   ${CMAKE_BINARY_DIR}/cmake/${PROJECT_NAME}-config-version.cmake)
write_basic_package_version_file(${VERSION_FILE_PATH}
    VERSION             ${PROJECT_VERSION}
    COMPATIBILITY       SameMajorVersion
)
install(FILES           ${VERSION_FILE_PATH} 
        DESTINATION     ${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}
)

#
# for testing, CTest will be used
#
if(NOT BUILD_TESTING)
    message(STATUS "Test is disabled.")
    return()
elseif(NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    message(STATUS "This is not a root project. Skipping the tests")
    return()
elseif(ANDROID OR IOS)
    return()
endif()
enable_testing()

# helper for test codes
set(BUILD_TESTING OFF)
add_subdirectory(external/latch)    
add_subdirectory(external/sockets)

set(CMAKE_CXX_STANDARD 20)

# create_ctest( ... )
function(create_ctest TEST_NAME)
    # create a test exe with the given name ...
    add_executable(${TEST_NAME} test/${TEST_NAME}.cpp)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    target_link_libraries(${TEST_NAME}
    PRIVATE
        Microsoft.GSL::GSL
    )
    # all arguments after TEST_NAME 
    # should be library (or CMake target) name
    foreach(idx RANGE 1 ${ARGC})
        target_link_libraries(${TEST_NAME}
        PRIVATE
            ${ARGV${idx}} 
        )
        if("${ARGV${idx}}" STREQUAL ssf)
            target_include_directories(${TEST_NAME}
            PRIVATE
                ${PROJECT_SOURCE_DIR}/external/sockets
            )    
        endif()
    endforeach()
    if(WIN32)	
        target_compile_definitions(${TEST_NAME}	
        PRIVATE	
            WIN32_LEAN_AND_MEAN NOMINMAX	
        )
    elseif(APPLE)	
        target_link_libraries(${TEST_NAME}	
        PRIVATE	
            c++	
        )	
    else(CMAKE_SYSTEM_NAME MATCHES Linux)	
        target_link_libraries(${TEST_NAME}
        PRIVATE	
            stdc++ c++
        )	
    endif()
endfunction()

create_ctest( article_russian_roulette  coroutine_portable )

#
#   <coroutine/yield.hpp>
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
# create_ctest( return_std_future           coroutine_portable )

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
create_ctest( windows_on_thread_pool        coroutine_system latch )

elseif(UNIX)
if(CMAKE_CXX_COMPILER_ID MATCHES GNU)
    # gcc-10 reports internal compiler error for these pthread tests
else()
create_ctest( pthread_detach_no_spawn       coroutine_system )
create_ctest( pthread_detach_spawn          coroutine_system )
create_ctest( pthread_join_no_spawn         coroutine_system )
create_ctest( pthread_join_spawn_1          coroutine_system )
create_ctest( pthread_join_spawn_2          coroutine_system )
endif()
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
#   <coroutine/channel.hpp>
#
if(CMAKE_CXX_COMPILER_ID MATCHES GNU)
    message(WARNING "using gcc: the compiler may not work for current code")
endif()
# create_ctest( channel_close_read            coroutine_system )
# create_ctest( channel_close_write           coroutine_system )
create_ctest( channel_ownership_consumer    coroutine_system )
create_ctest( channel_ownership_producer    coroutine_system )
create_ctest( channel_read_write_mutex      coroutine_system )
create_ctest( channel_read_write_nolock     coroutine_system )
create_ctest( channel_write_read_mutex      coroutine_system )
create_ctest( channel_write_read_nolock     coroutine_system )
# create_ctest( channel_select_empty          coroutine_system )
# create_ctest( channel_select_type           coroutine_system )
if(WIN32)
create_ctest( channel_race_condition        coroutine_system latch )
endif()
create_ctest( channel_sample_wrap           coroutine_system latch )

#
#   <coroutine/net.h>
#
if(TARGET coroutine_net)
create_ctest( net_socket_tcp_echo   coroutine_net ssf latch )
create_ctest( net_socket_udp_echo   coroutine_net ssf latch )
create_ctest( net_resolve_name      coroutine_net ssf )
create_ctest( net_resolve_ip6       coroutine_net ssf )
create_ctest( net_resolve_tcp6      coroutine_net ssf )
create_ctest( net_resolve_udp6      coroutine_net ssf )
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES Clang)
    add_test(NAME test_clang_1 COMMAND ${CMAKE_CXX_COMPILER} --version)
endif()
