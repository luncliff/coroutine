# ---------------------------------------------------------------------------
#
#   Author  : github.com/luncliff (luncliff@gmail.com)
#
# ---------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.5)

target_sources(${PROJECT_NAME}
PRIVATE
    thread/types.h
    thread/thread_data.cpp
    thread/thread_registry.cpp
)
