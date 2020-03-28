#
#   Author  : github.com/luncliff (luncliff@gmail.com)
#
add_library(coroutine_net
    # ${MODULE_INTERFACE_DIR}/coroutine/net.h
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

# if(NOT BUILD_SHARED_LIBS)
#     target_compile_definitions(coroutine_net
#     PUBLIC
#         FORCE_STATIC_LINK
#     )
# endif()

if(CMAKE_CXX_COMPILER_ID MATCHES Clang)
    if(WIN32)
        target_compile_options(coroutine_net
        PUBLIC
            /std:c++latest
            -fms-compatibility -Xclang -fcoroutines-ts
        )
        # in this case, we always use portable header
        set_target_properties(coroutine_net
        PROPERTIES
            PUBLIC_COMPILE_DEFINITIONS "USE_PORTABLE_COROUTINE_HANDLE;_RESUMABLE_FUNCTIONS_SUPPORTED"
        )
        
    elseif(UNIX OR APPLE)
        target_compile_options(coroutine_net
        PUBLIC
            -std=c++2a -stdlib=libc++
            -fcoroutines-ts
        )
    endif()

elseif(CMAKE_CXX_COMPILER_ID MATCHES GNU)
    target_compile_options(coroutine_net
    PUBLIC
        -std=gnu++2a
        -fcoroutines -fconcepts # -fno-exceptions
    )

elseif(MSVC)
    set_target_properties(coroutine_net
    PROPERTIES
        INTERFACE_LINK_FLAGS "${LINK_FLAGS} /errorReport:send"
    )
    target_compile_options(coroutine_net
    PUBLIC
        /std:c++latest /await
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
elseif(UNIX) # actually we can't sure it's a linux here ...
    target_sources(coroutine_net
    PRIVATE
        io_linux.cpp
    )
endif()
