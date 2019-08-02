
add_library(coroutine_portable INTERFACE)
add_library(coro::portable ALIAS coroutine_portable)

if(MSVC)
    set_target_properties(coroutine_portable
    PROPERTIES
        INTERFACE_LINK_FLAGS "${LINK_FLAGS} /errorReport:send"
    )
endif()

if(USE_PORTABLE_HEADER)
    message(STATUS "Customizaton")
    message(STATUS "  <coroutine/frame.h> is suppressing <experimental/coroutine>")
    message(STATUS)
    set_target_properties(coroutine_portable
    PROPERTIES
        INTERFACE_COMPILE_DEFINITIONS "USE_PORTABLE_COROUTINE_HANDLE"# ;_EXPERIMENTAL_RESUMABLE_
    )
endif()

target_include_directories(coroutine_portable
INTERFACE
    $<BUILD_INTERFACE:${BUILD_INTERFACE_DIR}>
    $<BUILD_INTERFACE:${GSL_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>
)

if(${CMAKE_CXX_COMPILER_ID} MATCHES Clang)
    if(WIN32)
        # 'target_compile_options' removes duplicated -Xclang directive.
        # avoide the removal using cmake flag variable
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Xclang -fcoroutines-ts" PARENT_SCOPE)
        target_compile_options(coroutine_portable
        INTERFACE
            /std:c++latest
            -fms-compatibility -Xclang -fcoroutines-ts
        )
        # in this case, we always use portable header
        set_target_properties(coroutine_portable
        PROPERTIES
            INTERFACE_COMPILE_DEFINITIONS "USE_PORTABLE_COROUTINE_HANDLE;_RESUMABLE_FUNCTIONS_SUPPORTED"
        )    
        
    elseif(UNIX)
        target_compile_options(coroutine_portable
        INTERFACE
            -std=c++2a -stdlib=libc++
            -fcoroutines-ts
        )
    endif()

elseif(MSVC)
    target_compile_options(coroutine_portable
    INTERFACE
        /std:c++latest /await
    )

elseif(${CMAKE_CXX_COMPILER_ID} MATCHES GNU)
    target_compile_options(coroutine_portable
    INTERFACE
        -std=gnu++2a
        -fcoroutines -fconcepts # -fno-exceptions
    )

endif()

install(FILES   ${BUILD_INTERFACE_DIR}/coroutine/frame.h
                ${BUILD_INTERFACE_DIR}/coroutine/return.h
                ${BUILD_INTERFACE_DIR}/coroutine/channel.hpp
        # Use DESTINATION instead to make sure of the path.
        # Must be same with INSTALL_INTERFACE above
        # TYPE    INCLUDE
        DESTINATION ${CMAKE_INSTALL_PREFIX}/include/coroutine
)
install(TARGETS         coroutine_portable
        EXPORT          coroutine-config
        RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
        LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
        ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
        # INCLUDES DESTINATION ${CMAKE_INSTALL_PREFIX}/include
)
