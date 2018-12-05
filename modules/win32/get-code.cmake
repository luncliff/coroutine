# ---------------------------------------------------------------------------
#
#   Author  : github.com/luncliff (luncliff@gmail.com)
#
# ---------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.5)

target_sources(${PROJECT_NAME}
PRIVATE
    win32/current_threads.cpp
    win32/net_aio.cpp
    win32/resolver.cpp
    win32/section.cpp
    win32/switch_to.cpp
    win32/wait_group.cpp
)

target_compile_definitions(${PROJECT_NAME}
PUBLIC
    NOMINMAX
)

# CMake variable MSVC follows WIN32. 
#   So we have to check clang first to be correct
if(${CMAKE_CXX_COMPILER_ID} MATCHES Clang)
    # Additional source code for clang
    target_sources(${PROJECT_NAME}
    PRIVATE
        win32/clang.cpp
    )

    # Need additional macro because this is not vcxproj
    target_compile_definitions(${PROJECT_NAME}
    PUBLIC
        _RESUMABLE_FUNCTIONS_SUPPORTED
    PRIVATE
        _WINDLL
    )

    # Argument for `clang-cl`
    #
    # `target_compile_options` removes duplicated -Xclang argument 
    # which must be protected. An alternative is to use CMAKE_CXX_FLAGS,
    # but the method will be used only when there is no way but to use it
    #
    # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Xclang -fcoroutines-ts")
    target_compile_options(${PROJECT_NAME} 
    PUBLIC
        /std:c++latest 
        -fms-compatibility 
        -Xclang -fcoroutines-ts
    PRIVATE
        -Wno-unused-function
        -Wno-c++98-compat 
        -Wno-reserved-id-macro 
        -Wno-missing-prototypes
    )
elseif(MSVC)
    target_compile_options(${PROJECT_NAME} 
    PUBLIC
        /std:c++latest 
        /await
    )

endif()

set_target_properties(${PROJECT_NAME}
PROPERTIES 
    LINK_FLAGS "${LINK_FLAGS} /errorReport:send"
)