# ---------------------------------------------------------------------------
#
#   Author  : github.com/luncliff (luncliff@gmail.com)
#
# ---------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.5)

# message(STATUS ${CMAKE_CXX_PLATFORM_ID})

target_sources(${PROJECT_NAME}
PRIVATE
    thread/types.h
    thread/compatible.cpp
    thread/${PLATFORM}.cpp
)
