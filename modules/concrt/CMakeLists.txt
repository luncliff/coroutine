#
#   Author  : github.com/luncliff (luncliff@gmail.com)
#
add_library(concrt
    ${BUILD_INTERFACE_DIR}/concurrency_helper.h
)
set_target_properties(concrt
PROPERTIES
    CXX_STANDARD    17
)
target_include_directories(concrt
PUBLIC
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

if(${CMAKE_CXX_COMPILER_ID} MATCHES GNU)
    if(UNIX)
        target_link_libraries(concrt
        PUBLIC
            pthread
        )
    endif()
else()
    target_link_libraries(concrt
    PUBLIC
        coro::thread
    )
endif()

if(${CMAKE_CXX_COMPILER_ID} MATCHES Clang)
    target_compile_options(concrt
    PRIVATE
        -Wall
        -Wno-unused-private-field
        -Wno-unused-function
        -Wno-c++98-compat
        -Wno-reserved-id-macro
        -Wno-missing-prototypes
        -ferror-limit=5
    )
    if(WIN32)
        if(BUILD_SHARED_LIBS)
            target_compile_definitions(concrt
            PRIVATE
                _WINDLL
            )
        endif()
    elseif(UNIX)
        target_compile_options(concrt
        PUBLIC
            -fPIC
        PRIVATE
            -fno-rtti -fvisibility=hidden
            # -fno-exceptions
        )
    endif()
elseif(${CMAKE_CXX_COMPILER_ID} MATCHES GNU)
    target_compile_options(concrt
    PRIVATE
        -Wall -Wno-unknown-pragmas
        -fno-rtti -fvisibility=hidden
        # -fno-exceptions
    )
elseif(MSVC)
    if(${CMAKE_BUILD_TYPE} MATCHES Debug)
        target_compile_options(concrt
        PRIVATE
            /Od
        )
    endif()
endif()


if(WIN32)
    target_sources(concrt
    PRIVATE
        latch_win32.cpp
        section_win32.cpp
    )
elseif(UNIX)
    target_sources(concrt
    PRIVATE
        latch_pthread.cpp
        section_pthread.cpp
    )
endif()
