#
#   Author  : github.com/luncliff (luncliff@gmail.com)
#
string(TOLOWER ${CMAKE_SYSTEM_NAME} SYSTEM_NAME)
if(SYSTEM_NAME MATCHES android)
    set(SYSTEM_NAME linux)
elseif(SYSTEM_NAME MATCHES ios)
    set(SYSTEM_NAME darwin)
endif()

add_library(coroutine_system
    libmain.cpp
    ${SYSTEM_NAME}.cpp
)

set_target_properties(coroutine_system
PROPERTIES
    CXX_STANDARD    17
    WINDOWS_EXPORT_ALL_SYMBOLS  true
)

target_include_directories(coroutine_system
PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
PUBLIC
    $<BUILD_INTERFACE:${MODULE_INTERFACE_DIR}>
    $<BUILD_INTERFACE:${GSL_INCLUDE_DIR}>
)

target_link_libraries(coroutine_system
PUBLIC
    coroutine_portable
PRIVATE
    ${CMAKE_DL_LIBS}
)

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(coroutine_system
    PUBLIC
        FORCE_STATIC_LINK
    )
endif()

if(WIN32)
    target_compile_definitions(coroutine_system
    PRIVATE
        WIN32_LEAN_AND_MEAN NOMINMAX
    )
    target_link_libraries(coroutine_system
    PUBLIC
        kernel32
    )
elseif(APPLE)
    target_sources(coroutine_system
    PRIVATE
        kqueue.cpp
        pthread.cpp
    )
    target_link_libraries(coroutine_system
    PUBLIC
        pthread c++
    )

elseif(CMAKE_SYSTEM_NAME MATCHES Linux)
    target_sources(coroutine_system
    PRIVATE
        pthread.cpp
    )
    if(ANDROID)
        target_link_libraries(coroutine_system
        PUBLIC
            ${ANDROID_STL}
        )
    else()
        target_link_libraries(coroutine_system
        PUBLIC
            rt pthread
        )
    endif()
endif()

install(TARGETS         coroutine_system
        EXPORT          coroutine-config
        INCLUDES DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
        RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
        LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
        ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
)
