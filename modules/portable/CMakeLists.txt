
add_library(portable INTERFACE)

target_include_directories(portable
INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>
)

if(${CMAKE_CXX_COMPILER_ID} MATCHES Clang)
    if(WIN32)
        # cmake removes duplicated -Xclang directive.
        # avoide the removal using cmake flag variable
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Xclang -fcoroutines-ts")
        target_compile_options(portable
        INTERFACE
            /std:c++latest
            -fms-compatibility
        )

    elseif(UNIX)
        target_compile_options(portable
        INTERFACE
            -std=c++2a -fcoroutines-ts
            -stdlib=libc++
        )
    endif()

elseif(MSVC)
    target_compile_options(portable
    INTERFACE
        /std:c++latest /await
    )

elseif(${CMAKE_CXX_COMPILER_ID} MATCHES GNU)
    target_compile_options(portable
    INTERFACE
        -fcoroutine -fconcepts
        -std=gnu++2a
    )

endif()

install(FILES   ${PROJECT_SOURCE_DIR}/include/frame.h
        # Use DESTINATION instead to make sure of the path.
        # Must be same with INSTALL_INTERFACE above
        # TYPE    INCLUDE
        DESTINATION ${CMAKE_INSTALL_PREFIX}/include
)
install(TARGETS         portable
        EXPORT          coroutine-config
        RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
        LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
        ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
        # INCLUDES DESTINATION ${CMAKE_INSTALL_PREFIX}/include
)
