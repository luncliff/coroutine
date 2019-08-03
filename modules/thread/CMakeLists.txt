#
#   Author  : github.com/luncliff (luncliff@gmail.com)
#
add_library(coroutine_thread
    ${BUILD_INTERFACE_DIR}/coroutine/thread.h
)
add_library(coro::thread ALIAS coroutine_thread)

set_target_properties(coroutine_thread
PROPERTIES
    CXX_STANDARD    17
)

target_include_directories(coroutine_thread
PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(coroutine_thread
PUBLIC
    coro::portable
PRIVATE
    ${CMAKE_DL_LIBS}
)

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(coroutine_thread
    PUBLIC
        FORCE_STATIC_LINK
    )
endif()

if(${CMAKE_CXX_COMPILER_ID} MATCHES Clang)
    target_compile_options(coroutine_thread
    PRIVATE
        -Wall
        -Wno-unused-private-field
        -Wno-unused-function
        -Wno-c++98-compat
        -Wno-reserved-id-macro
        -Wno-missing-prototypes
    )
    if(WIN32)
    elseif(UNIX)
        target_link_libraries(coroutine_thread
        PUBLIC
            c++
        )
        target_compile_options(coroutine_thread
        PUBLIC
            -fPIC
        PRIVATE
            -fno-rtti -fvisibility=hidden
            -ferror-limit=5
            # -fno-exceptions
        )
    endif()
elseif(${CMAKE_CXX_COMPILER_ID} MATCHES GNU)
    target_link_libraries(coroutine_thread
    PUBLIC
        stdc++
    )
    target_compile_options(coroutine_thread
    PUBLIC
        -fPIC
    PRIVATE
        -Wall -Wno-unknown-pragmas
        -fno-rtti -fvisibility=hidden
        # -fno-exceptions
    )
elseif(MSVC)
    target_compile_options(coroutine_thread
    PRIVATE
        /W4
    )
    if(${CMAKE_BUILD_TYPE} MATCHES Debug)
        target_compile_options(coroutine_thread
        PRIVATE
            /Od
        )
    endif()
endif()

if(ANDROID)
    target_link_libraries(coroutine_thread
    PUBLIC
        ${ANDROID_STL}
    )
elseif(WIN32)
    target_sources(coroutine_thread
    PRIVATE
        ptp_work.cpp
    )
    target_compile_definitions(coroutine_thread
    PUBLIC
        WIN32_LEAN_AND_MEAN NOMINMAX
    )
elseif(UNIX) # actually we can't sure it's a linux here ...
    target_sources(coroutine_thread
    PRIVATE
        pthread_joiner.cpp
    )
    target_link_libraries(coroutine_thread
    PUBLIC
        pthread
    )
endif()

if(NOT MSVC)
    if(${CMAKE_BUILD_TYPE} MATCHES Debug)
        # code coverage option can lead to compiler crash
        # list(APPEND CMAKE_CXX_FLAGS "--coverage")
        target_compile_options(coroutine_thread
        PRIVATE
            -g -O0
        )
    else()
        target_compile_options(coroutine_thread
        PRIVATE
            -O3
        )
    endif()
endif()

install(FILES       ${BUILD_INTERFACE_DIR}/coroutine/thread.h
        DESTINATION ${CMAKE_INSTALL_PREFIX}/include/coroutine
)
install(TARGETS         coroutine_thread
        EXPORT          coroutine-config
        RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
        LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
        ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
)
