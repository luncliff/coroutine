# ---------------------------------------------------------------------------
#
#   Author  : github.com/luncliff (luncliff@gmail.com)
#
# ---------------------------------------------------------------------------

if(NOT UNIX)
    message(FATAL_ERROR "Expect UNIX platform. Current platform is ${CMAKE_SYSTEM}")
endif()

add_library(${PROJECT_NAME}
    darwin/dllmain.cpp
    darwin/kernel_queue.cpp
    darwin/kernel_queue.h
    darwin/event.cpp
    darwin/net.cpp

    posix/concrt.cpp
    net/resolver.cpp
)

target_compile_options(${PROJECT_NAME}
PUBLIC
    -std=c++2a
    -stdlib=libc++
    -fcoroutines-ts -fPIC
PRIVATE
    -Wall -Wno-unknown-pragmas -Wno-unused-private-field
    -fvisibility=hidden -fno-rtti
    -ferror-limit=5
)

if(${CMAKE_BUILD_TYPE} MATCHES Debug)
    # code coverage option can lead to compiler crash
    # list(APPEND CMAKE_CXX_FLAGS "--coverage")
    target_compile_options(${PROJECT_NAME}
    PRIVATE
        -g -O0
    )
else()
    target_compile_options(${PROJECT_NAME}
    PRIVATE
        -O3
    )
endif()
