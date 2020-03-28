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

if(USE_PORTABLE_HEADER)
    message(STATUS "Customizaton")
    message(STATUS "  <coroutine/frame.h> is suppressing <experimental/coroutine>")
    message(STATUS)
    target_compile_definitions(coroutine_portable
    PUBLIC
        USE_PORTABLE_COROUTINE_HANDLE
    )
endif()

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
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Xclang -fcoroutines-ts" PARENT_SCOPE)
        target_compile_options(coroutine_portable
        PUBLIC
            /std:c++latest
            -fms-compatibility -Xclang -fcoroutines-ts
        )
        # in this case, we always use portable header
        target_compile_definitions(coroutine_portable
        PUBLIC
            USE_PORTABLE_COROUTINE_HANDLE
            _RESUMABLE_FUNCTIONS_SUPPORTED
            # _EXPERIMENTAL_RESUMABLE_
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
    # set_target_properties(coroutine_portable
    # PROPERTIES
    #     INTERFACE_LINK_FLAGS "${LINK_FLAGS} /errorReport:send"
    # )
    target_compile_options(coroutine_portable
    PUBLIC
        /std:c++latest /await
    PRIVATE
        /JMC-
    )

endif()

if(WIN32)
    target_link_libraries(coroutine_portable
    PUBLIC
        kernel32 ${CMAKE_DL_LIBS}
    )

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
