# ---------------------------------------------------------------------------
#
#   Author  : github.com/luncliff (luncliff@gmail.com)
#
# ---------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.5)

target_sources(${PROJECT_NAME}
PRIVATE
    messaging/concurrent.h
    messaging/operations.cpp
    messaging/queue.cpp
)
