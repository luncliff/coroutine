# ---------------------------------------------------------------------------
#
#   Author  : github.com/luncliff (luncliff@gmail.com)
#
# ---------------------------------------------------------------------------

if(NOT UNIX)
    message(FATAL_ERROR "Expect UNIX platform. Current platform is ${CMAKE_SYSTEM}")
endif()

# Rely on POSIX API
target_sources(${PROJECT_NAME}
PRIVATE
    ${CMAKE_SYSTEM_NAME}/switch_to.cpp
    ${CMAKE_SYSTEM_NAME}/wait_group.cpp
    ${CMAKE_SYSTEM_NAME}/section.cpp
    ${CMAKE_SYSTEM_NAME}/sync.cpp
)

target_compile_options(${PROJECT_NAME} 
PUBLIC
    -std=c++2a
    -stdlib=libc++
    -fcoroutines-ts -fPIC 
PRIVATE
    -Wall -Wno-unknown-pragmas -Wno-unused-private-field
    -fvisibility=hidden -fno-rtti 
)

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

target_link_libraries(${PROJECT_NAME}
PUBLIC
    pthread
)

if(APPLE) # macos custom

elseif(LINUX) # linux custom

    target_link_libraries(${PROJECT_NAME}
    PUBLIC
        rt c++ # c++abi c++experimental
    )
endif()
