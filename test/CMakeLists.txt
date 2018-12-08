# ---------------------------------------------------------------------------
#
#   Author  : github.com/luncliff (luncliff@gmail.com)
#
# ---------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.5)

# testing the library 
add_executable(coroutine_test
    entry.cpp
    
    generic/unplug.cpp
    # generic/channel.cpp
    generic/generator.cpp
    generic/async_generator.cpp

    thread/message.cpp
    thread/switch_to.cpp
    thread/sync.cpp
)

# set_target_properties(coroutine_test
# PROPERTIES 
#     CXX_STANDARD    17
# )

target_include_directories(coroutine_test
PRIVATE
    ${PROJECT_DIR}/test
)
target_link_libraries(coroutine_test
PRIVATE
    coroutine
)

if(UNIX)
    target_link_libraries(coroutine_test
    PRIVATE
        stdc++ # solve possible gnu-dependency
    )
    target_compile_options(coroutine_test
    PRIVATE
        -std=c++2a -stdlib=libc++ 
    )
endif()
