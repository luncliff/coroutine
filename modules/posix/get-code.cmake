# ---------------------------------------------------------------------------
#
#   Author  : github.com/luncliff (luncliff@gmail.com)
#
# ---------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.5)

if(NOT UNIX)
    message(FATAL_ERROR "Expect UNIX platform. Current platform is ${CMAKE_SYSTEM}")
endif()

# Rely on POSIX API
target_sources(${PROJECT_NAME}
PRIVATE
    posix/switch_to.cpp
    posix/wait_group.cpp
    posix/background.cpp
)
target_compile_options(${PROJECT_NAME} 
PUBLIC
    -stdlib=libc++
    -fcoroutines-ts -fPIC 
PRIVATE
    -Wall -Wno-unknown-pragmas -Wno-unused-private-field
    -fvisibility=hidden -fno-rtti 
)
target_link_libraries(${PROJECT_NAME}
PUBLIC
    pthread
)

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
        rt c++ # c++abi c++experimental
    )
endif()
