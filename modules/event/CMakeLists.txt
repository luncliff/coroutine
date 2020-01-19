#
#   Author  : github.com/luncliff (luncliff@gmail.com)
#
add_library(coroutine_event
    ${MODULE_INTERFACE_DIR}/coroutine/event_${CMAKE_SYSTEM_NAME}.h
    libmain.cpp
    event_${CMAKE_SYSTEM_NAME}.cpp
)

set_target_properties(coroutine_event
PROPERTIES
    CXX_STANDARD    17
    PUBLIC_HEADER   "${MODULE_INTERFACE_DIR}/coroutine/event_${CMAKE_SYSTEM_NAME}.h"
    WINDOWS_EXPORT_ALL_SYMBOLS  true
)

target_include_directories(coroutine_event
PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
PUBLIC
    $<BUILD_INTERFACE:${MODULE_INTERFACE_DIR}>
    $<BUILD_INTERFACE:${GSL_INCLUDE_DIR}>
)

target_link_libraries(coroutine_event
PUBLIC
    coroutine_portable
PRIVATE
    ${CMAKE_DL_LIBS}
)

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(coroutine_event
    PUBLIC
        FORCE_STATIC_LINK
    )
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES Clang)
    if(WIN32)
        target_compile_options(coroutine_event
        PUBLIC
            /std:c++latest
            -fms-compatibility -Xclang -fcoroutines-ts
        )
        # in this case, we always use portable header
        set_target_properties(coroutine_event
        PROPERTIES
            PUBLIC_COMPILE_DEFINITIONS "USE_PORTABLE_COROUTINE_HANDLE;_RESUMABLE_FUNCTIONS_SUPPORTED"
        )
        
    elseif(UNIX OR APPLE)
        target_compile_options(coroutine_event
        PUBLIC
            -std=c++2a -stdlib=libc++
            -fcoroutines-ts
        )
    endif()

elseif(CMAKE_CXX_COMPILER_ID MATCHES GNU)
    target_compile_options(coroutine_event
    PUBLIC
        -std=gnu++2a
        -fcoroutines -fconcepts # -fno-exceptions
    )

elseif(MSVC)
    set_target_properties(coroutine_event
    PROPERTIES
        INTERFACE_LINK_FLAGS "${LINK_FLAGS} /errorReport:send"
    )
    target_compile_options(coroutine_event
    PUBLIC
        /std:c++latest /await
    )

endif()

if(WIN32)
    target_compile_definitions(coroutine_event
    PRIVATE
        WIN32_LEAN_AND_MEAN NOMINMAX
    )
    target_link_libraries(coroutine_event
    PUBLIC
        kernel32
    )
elseif(APPLE)
    target_sources(coroutine_event
    PRIVATE
        kernel_queue.h
        kernel_queue.cpp
    )
elseif(UNIX) # actually we can't sure it's a linux here ...
    target_sources(coroutine_event
    PRIVATE
        event_poll.h
        event_poll.cpp
    )
    if(ANDROID)
        target_link_libraries(coroutine_event
        PUBLIC
            ${ANDROID_STL}
        )
    else()
        target_link_libraries(coroutine_event
        PUBLIC
            rt
        )
    endif()
endif()

install(TARGETS         coroutine_event
        EXPORT          coroutine-config
        INCLUDES DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
        RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
        LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
        ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
)
