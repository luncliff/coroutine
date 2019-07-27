#
#   Author  : github.com/luncliff (luncliff@gmail.com)
#
add_library(concrt
    ${INTERFACE_DIR}/concurrency_helper.h
)
set_target_properties(concrt
PROPERTIES
    CXX_STANDARD    17
)
target_include_directories(concrt
PUBLIC
    ${INTERFACE_DIR}
    ${GSL_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(concrt
    PUBLIC
        FORCE_STATIC_LINK
    )
endif()

if(MSVC)
    target_compile_options(concrt
    PRIVATE
        /W4
    )
    target_compile_definitions(concrt
    PUBLIC
        WIN32_LEAN_AND_MEAN NOMINMAX
    )
else(${CMAKE_CXX_COMPILER_ID} MATCHES GNU)
    target_compile_options(concrt
    PUBLIC
        -fPIC -Wall
    )
endif()

if(WIN32)
    target_sources(concrt
    PRIVATE
        sync_windows.cpp
    )
elseif(APPLE)
    target_sources(concrt
    PRIVATE
        sync_darwin.cpp
        latch_pthread.cpp
    )
elseif(UNIX) # actually we can't sure it's a linux here ...
    target_sources(concrt
    PRIVATE
        sync_linux.cpp
        latch_pthread.cpp
    )
endif()
