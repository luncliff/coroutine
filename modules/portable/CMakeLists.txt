#
#   Author  : github.com/luncliff (luncliff@gmail.com)
#
add_library(coroutine_portable
    ${MODULE_INTERFACE_DIR}/coroutine/frame.h
    frame.cpp
)
set_target_properties(coroutine_portable
PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS true
)

target_include_directories(coroutine_portable
PUBLIC
    $<BUILD_INTERFACE:${MODULE_INTERFACE_DIR}>
    $<BUILD_INTERFACE:${GSL_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>
)

if(CMAKE_CXX_COMPILER_ID MATCHES Clang)
    if(WIN32)
        # 'target_compile_options' removes duplicated -Xclang directive.
        # avoide the removal using cmake flag variable
        target_compile_options(coroutine_portable
        PUBLIC
            "/clang:-fcoroutines-ts"
            -fms-compatibility
        )
        # in this case, we always use portable header
        target_compile_definitions(coroutine_portable
        PUBLIC
            USE_PORTABLE_COROUTINE_HANDLE
            _RESUMABLE_FUNCTIONS_SUPPORTED  # _EXPERIMENTAL_RESUMABLE_
        PRIVATE
            FORCE_CLANG
        )

    elseif(UNIX OR APPLE)
        target_compile_options(coroutine_portable
        PUBLIC
            -std=c++2a -stdlib=libc++
            -fcoroutines-ts
        )
    endif()

elseif(CMAKE_CXX_COMPILER_ID MATCHES GNU)
    target_compile_options(coroutine_portable
    PUBLIC
        -std=gnu++2a
        -fcoroutines -fconcepts # -fno-exceptions
    )

elseif(MSVC)
    # select between C++ Coroutines TS & C++ 20 Coroutines
    if(support_intrinsic_builtin OR has_coroutine)
        target_compile_options(coroutine_portable
        PUBLIC
            /std:c++latest
        )
    else()
        target_compile_options(coroutine_portable
        PUBLIC
            /std:c++17 /await
        )
    endif()
    target_compile_options(coroutine_portable
    PRIVATE
        /JMC-
    )
endif()

if(WIN32)
elseif(CMAKE_SYSTEM_NAME MATCHES Linux)
    if(ANDROID)
        target_link_libraries(coroutine_portable
        PUBLIC
            ${ANDROID_STL}
        )
    else()
        target_link_libraries(coroutine_portable
        PUBLIC
            stdc++
        )
    endif()

elseif(UNIX OR APPLE)
    target_link_libraries(coroutine_portable
    PUBLIC
        c++
    )

endif()
