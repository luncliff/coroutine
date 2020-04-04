#
#   Author  : github.com/luncliff (luncliff@gmail.com)
#
add_library(coroutine_net
    ${MODULE_INTERFACE_DIR}/coroutine/net.h
    resolver.cpp
)

set_target_properties(coroutine_net
PROPERTIES
    CXX_STANDARD    17
    WINDOWS_EXPORT_ALL_SYMBOLS  true
)

target_include_directories(coroutine_net
PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
PUBLIC
    $<BUILD_INTERFACE:${MODULE_INTERFACE_DIR}>
    $<BUILD_INTERFACE:${GSL_INCLUDE_DIR}>
)

target_link_libraries(coroutine_net
PUBLIC
    coroutine_portable
    coroutine_system
PRIVATE
    ${CMAKE_DL_LIBS}
)

if(CMAKE_CXX_COMPILER_ID MATCHES Clang)
    # ... PRIVATE should be placed here ...

elseif(CMAKE_CXX_COMPILER_ID MATCHES GNU)
    # ... PRIVATE should be placed here ...

elseif(MSVC)
    set_target_properties(coroutine_net
    PROPERTIES
        INTERFACE_LINK_FLAGS "${LINK_FLAGS} /errorReport:send"
    )
    target_compile_options(coroutine_net
    PRIVATE
        /JMC-
    )

endif()

if(WIN32)
    target_sources(coroutine_net
    PRIVATE
        io_windows.cpp
    )
    target_link_libraries(coroutine_net
    PUBLIC
        kernel32 ws2_32 mswsock
    )
elseif(APPLE)
    target_sources(coroutine_net
    PRIVATE
        io_darwin.cpp
    )
elseif(CMAKE_SYSTEM_NAME MATCHES Linux)
    target_sources(coroutine_net
    PRIVATE
        io_linux.cpp
    )
endif()
