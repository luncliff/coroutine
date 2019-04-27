# ---------------------------------------------------------------------------
#
#   Author  : github.com/luncliff (luncliff@gmail.com)
#
# ---------------------------------------------------------------------------

add_library(${PROJECT_NAME}
    windows/concrt.cpp
    windows/net.cpp
    net/resolver.cpp
)
target_compile_definitions(${PROJECT_NAME}
PUBLIC
    NOMINMAX
)

# CMake variable MSVC follows windows.
#   So we have to check clang first to be correct
if(${CMAKE_CXX_COMPILER_ID} MATCHES Clang)
    # Additional source code for clang
    target_sources(${PROJECT_NAME}
    PRIVATE
        windows/clang.cpp
    )

    # Need additional macro because this is not vcxproj
    target_compile_definitions(${PROJECT_NAME}
    PUBLIC
        _RESUMABLE_FUNCTIONS_SUPPORTED
    )
    
    
    if(BUILD_SHARED_LIBS)
        message(FATAL_ERROR "clang-cl build need to use static linking for current version")
    endif()

    # clang-cl build failes for this condition.
    # finding a solution, but can't sure about it...
    target_compile_definitions(${PROJECT_NAME}
    PUBLIC
        USE_STATIC_LINK_MACRO
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
        -Wno-unused-private-field
        -Wno-unused-function
        -Wno-c++98-compat
        -Wno-reserved-id-macro
        -Wno-missing-prototypes
    )
elseif(MSVC)
    target_compile_options(${PROJECT_NAME}
    PUBLIC
        /std:c++latest /await
        /W4
    )
endif()

if(${CMAKE_BUILD_TYPE} MATCHES Debug)
    target_compile_options(${PROJECT_NAME}
    PRIVATE
        /Od
    )
endif()

set_target_properties(${PROJECT_NAME}
PROPERTIES
    LINK_FLAGS "${LINK_FLAGS} /errorReport:send"
)
