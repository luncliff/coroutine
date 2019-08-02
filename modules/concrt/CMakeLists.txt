#
#   Author  : github.com/luncliff (luncliff@gmail.com)
#
add_library(concrt
    ${BUILD_INTERFACE_DIR}/concurrency_helper.h
)
# set_target_properties(concrt
# PROPERTIES
#     CXX_STANDARD    17
# )
target_include_directories(concrt
PUBLIC
    ${BUILD_INTERFACE_DIR}
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
        /std:c++latest /W4 
    )
    target_compile_definitions(concrt
    PUBLIC
        WIN32_LEAN_AND_MEAN NOMINMAX
    )
elseif(${CMAKE_CXX_COMPILER_ID} MATCHES Clang)
    target_compile_options(concrt
    PUBLIC
        -std=c++2a -stdlib=libc++
        -Wall -Wno-unknown-pragmas
        -fPIC -fcoroutines-ts -fno-rtti
    )
elseif(${CMAKE_CXX_COMPILER_ID} MATCHES GNU)
    target_compile_options(concrt
    PUBLIC
        -std=gnu++2a
        -Wall
        -fPIC -fcoroutines -fno-rtti # -fno-exceptions 
    )
    target_link_libraries(concrt
    PUBLIC
        stdc++
    )
endif()

if(WIN32)
    target_sources(concrt
    PRIVATE
        sync_windows.cpp
    )
elseif(UNIX)
    target_sources(concrt
    PRIVATE
        latch_pthread.cpp
        section_pthread.cpp
    )
endif()
