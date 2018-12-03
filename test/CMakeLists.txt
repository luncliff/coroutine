# ---------------------------------------------------------------------------
#
#   Author  : github.com/luncliff (luncliff@gmail.com)
#
# ---------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.5)

# testing the library 
add_executable(coroutine_test
    entry.cpp
    
    # resource/registry.cpp

    generic/unplug.cpp
    generic/channel.cpp
    generic/generator.cpp
    generic/sequence.cpp

    thread/message.cpp
    thread/switch_to.cpp
    thread/section.cpp
)

set_target_properties(coroutine_test
PROPERTIES 
    CXX_STANDARD    17
)

target_include_directories(coroutine_test
PRIVATE
    ${PROJECT_DIR}/test
)
target_link_libraries(coroutine_test
PUBLIC
    coroutine
)

if(NOT WIN32)
    target_link_libraries(coroutine_test
    PUBLIC
        stdc++ # solve possible gnu-dependency
    )
endif()
