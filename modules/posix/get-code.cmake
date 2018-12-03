# ---------------------------------------------------------------------------
#
#   Author  : github.com/luncliff (luncliff@gmail.com)
#
# ---------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.5)

target_sources(${PROJECT_NAME}
PRIVATE
    posix/switch_to.cpp
    posix/wait_group.cpp
)

if(UNIX) # POSIX API
    target_sources(${PROJECT_NAME}
    PRIVATE
        posix/adapter.h
        posix/condvar.cpp
        posix/switch_to_impl.cpp
    )
    target_compile_options(${PROJECT_NAME} 
    PUBLIC
        -stdlib=libc++    -fPIC 
        -fcoroutines-ts
    PRIVATE
        -Wall -Wno-unknown-pragmas
        -fno-rtti 
        -fvisibility=hidden
    )
    target_link_libraries(${PROJECT_NAME}
    PUBLIC
        pthread
    )
endif()

if(APPLE) # macos custom
    target_sources(${PROJECT_NAME}
    PRIVATE
        posix/section.osx.cpp
    )
elseif(LINUX) # linux custom
    target_sources(${PROJECT_NAME}
    PRIVATE
        posix/section.linux.cpp
    )
    target_link_libraries(${PROJECT_NAME}
    PUBLIC
        rt
        libc++.so libc++abi.so libc++experimental.a 
    )
endif()
