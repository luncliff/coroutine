# ---------------------------------------------------------------------------
#
#   Author  : github.com/luncliff (luncliff@gmail.com)
#
# ---------------------------------------------------------------------------

if(NOT UNIX)
    message(FATAL_ERROR "Expect UNIX platform. Current platform is ${CMAKE_SYSTEM}")
endif()

add_library(${PROJECT_NAME}
    linux/dllmain.cpp

    linux/event_poll.h
    linux/event_poll.cpp
    linux/event.cpp
    
    posix/concrt.cpp
    linux/net.cpp
    net/resolver.cpp
)

if(${CMAKE_CXX_COMPILER_ID} MATCHES Clang)
    target_compile_options(${PROJECT_NAME}
    PUBLIC
        -std=c++2a -stdlib=libc++ -fcoroutines-ts 
        -fPIC
    PRIVATE
        -Wall -Wno-unknown-pragmas -Wno-unused-private-field
        -fno-rtti -fvisibility=hidden -ferror-limit=5
    )
elseif(${CMAKE_CXX_COMPILER_ID} MATCHES GNU)
    target_compile_options(${PROJECT_NAME}
    PUBLIC
        -std=gnu++2a -fcoroutines # -fno-exceptions 
        -fPIC
    PRIVATE
        -Wall -Wno-unknown-pragmas
        -fno-rtti -fvisibility=hidden
    )
endif()

if(${CMAKE_BUILD_TYPE} MATCHES Debug)
    # code coverage option lead to compiler crash
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

if(ANDROID)
    target_link_libraries(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_DL_LIBS} ${ANDROID_STL}
    )
else()
    target_link_libraries(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_DL_LIBS} pthread rt c++ # c++abi c++experimental
    )
endif()
