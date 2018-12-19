# ---------------------------------------------------------------------------
#
#   Author  : github.com/luncliff (luncliff@gmail.com)
#
# ---------------------------------------------------------------------------

target_sources(${PROJECT_NAME}
PRIVATE
    thread/types.h
    thread/compatible.cpp
    thread/registry_${PLATFORM}.cpp

    thread/queue.h
    thread/queue.cpp
    thread/operations.cpp
)
