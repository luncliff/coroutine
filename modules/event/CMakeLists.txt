#
#   Author  : github.com/luncliff (luncliff@gmail.com)
#
add_library(coroutine_event
    ${BUILD_INTERFACE_DIR}/coroutine/event.h
    libmain.cpp
)
add_library(coro::event ALIAS coroutine_event)

set_target_properties(coroutine_event
PROPERTIES
    CXX_STANDARD    17
)

target_include_directories(coroutine_event
PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(coroutine_event
PUBLIC
    coro::portable
PRIVATE
    ${CMAKE_DL_LIBS}
)

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(coroutine_event
    PUBLIC
        FORCE_STATIC_LINK
    )
endif()

if(${CMAKE_CXX_COMPILER_ID} MATCHES Clang)
    target_compile_options(coroutine_event
    PRIVATE
        -Wall
        -Wno-unused-private-field
        -Wno-unused-function
        -Wno-c++98-compat
        -Wno-reserved-id-macro
        -Wno-missing-prototypes
        -ferror-limit=5
    )
    if(WIN32)
        if(BUILD_SHARED_LIBS)
            target_compile_definitions(coroutine_event
            PRIVATE
                _WINDLL
            )
        endif()
    elseif(UNIX)
        target_link_libraries(coroutine_event
        PUBLIC
            c++
        )
        target_compile_options(coroutine_event
        PUBLIC
            -fPIC
        PRIVATE
            -fno-rtti -fvisibility=hidden
            # -fno-exceptions
        )
    endif()
elseif(${CMAKE_CXX_COMPILER_ID} MATCHES GNU)
    target_link_libraries(coroutine_event
    PUBLIC
        stdc++
    )
    target_compile_options(coroutine_event
    PUBLIC
        -fPIC
    PRIVATE
        -Wall -Wno-unknown-pragmas
        -fno-rtti -fvisibility=hidden
        # -fno-exceptions
    )
elseif(MSVC)
    target_compile_options(coroutine_event
    PRIVATE
        /W4
    )
    if(${CMAKE_BUILD_TYPE} MATCHES Debug)
        target_compile_options(coroutine_event
        PRIVATE
            /Od
        )
    endif()
endif()

if(ANDROID)
    target_link_libraries(coroutine_event
    PUBLIC
        ${ANDROID_STL}
    )
elseif(WIN32)
    target_sources(coroutine_event
    PRIVATE
        ptp_event.cpp
    )
    target_compile_definitions(coroutine_event
    PUBLIC
        WIN32_LEAN_AND_MEAN NOMINMAX
    )
elseif(APPLE)
    target_sources(coroutine_event
    PRIVATE
        kernel_queue.h
        kernel_queue.cpp
        event_darwin.cpp
    )
elseif(UNIX) # actually we can't sure it's a linux here ...
    target_sources(coroutine_event
    PRIVATE
        event_poll.h
        event_poll.cpp
        event_linux.cpp
    )
    target_link_libraries(coroutine_event
    PUBLIC
        rt
    )
endif()

if(NOT MSVC)
    if(${CMAKE_BUILD_TYPE} MATCHES Debug)
        # code coverage option can lead to compiler crash
        # list(APPEND CMAKE_CXX_FLAGS "--coverage")
        target_compile_options(coroutine_event
        PRIVATE
            -g -O0
        )
    else()
        target_compile_options(coroutine_event
        PRIVATE
            -O3
        )
    endif()
endif()

install(FILES       ${BUILD_INTERFACE_DIR}/coroutine/event.h
        DESTINATION ${CMAKE_INSTALL_PREFIX}/include/coroutine
)
install(TARGETS         coroutine_event
        EXPORT          coroutine-config
        RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
        LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
        ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
)
