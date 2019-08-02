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

target_link_libraries(concrt
PUBLIC
    coro::portable
)

if(WIN32)
    target_sources(concrt
    PRIVATE
        sync_windows.cpp
    )

    target_compile_definitions(concrt
    PUBLIC
        WIN32_LEAN_AND_MEAN NOMINMAX
    )
    if(${CMAKE_CXX_COMPILER_ID} MATCHES Clang)
        target_compile_options(concrt
        PUBLIC
            -Wall -Wno-unknown-pragmas
            -fPIC -fno-rtti
        )
    elseif(MSVC)
        target_compile_options(concrt
        PRIVATE
            /W4 
        )
    endif()
elseif(UNIX)
    target_sources(concrt
    PRIVATE
        latch_pthread.cpp
        section_pthread.cpp
    )

    if(${CMAKE_CXX_COMPILER_ID} MATCHES Clang)
        target_compile_options(concrt
        PUBLIC
            -Wall -Wno-unknown-pragmas
            -fPIC -fno-rtti
        )
    elseif(${CMAKE_CXX_COMPILER_ID} MATCHES GNU)
        target_compile_options(concrt
        PUBLIC
            -Wall
            -fPIC -fno-rtti # -fno-exceptions 
        )
        target_link_libraries(concrt
        PUBLIC
            stdc++
        )
    endif()
endif()


