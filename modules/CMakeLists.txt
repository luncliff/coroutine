# ---------------------------------------------------------------------------
#
#   Author  : github.com/luncliff (luncliff@gmail.com)
#
# ---------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.8)

# redirect to ${target-platform}.cmake for system-specific configurations
string(TOLOWER ${CMAKE_SYSTEM_NAME} TARGET_PLATFORM_NAME)
list(APPEND CMAKE_MODULE_PATH 
            ${CMAKE_CURRENT_SOURCE_DIR}
)
if(ANDROID)
    set(TARGET_PLATFORM_NAME linux)
elseif(IOS)
    set(TARGET_PLATFORM_NAME darwin)
endif()
include(${TARGET_PLATFORM_NAME})
add_library(coro ALIAS ${PROJECT_NAME})

# shared configurations ...
set_target_properties(${PROJECT_NAME}
PROPERTIES
    CXX_STANDARD    17
    SOVERSION       ${PROJECT_VERSION}
)
target_include_directories(${PROJECT_NAME}
PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../interface>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>
PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${GSL_INCLUDE_DIR}  # since the library is header-only, 
                        # end user will manage the path properly
)

if(USE_CUSTOM_HEADER)
    message(STATUS "using custom coroutine header")
    target_compile_definitions(${PROJECT_NAME}
    PUBLIC
        USE_CUSTOM_HEADER
    )
endif()

# library export / install
include(CMakePackageConfigHelpers)
set(VERSION_FILE        ${CMAKE_BINARY_DIR}/cmake/coroutine-config-version.cmake)
write_basic_package_version_file(${VERSION_FILE}
    VERSION             ${PROJECT_VERSION}
    COMPATIBILITY       ExactVersion # no compatibility for now
)

install(DIRECTORY       ${PROJECT_SOURCE_DIR}/interface/coroutine
        DESTINATION     ${CMAKE_INSTALL_PREFIX}/include
)
install(FILES           ${VERSION_FILE}
        DESTINATION     ${CMAKE_INSTALL_PREFIX}/share/coroutine
)
if(WIN32)
    install(TARGETS         ${PROJECT_NAME}
            EXPORT          coroutine-config    # --> config for find_package
            RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
            LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
            ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
    )
else()
    install(TARGETS         ${PROJECT_NAME}
            EXPORT          coroutine-config
            DESTINATION     ${CMAKE_INSTALL_PREFIX}/lib
    )
endif()
install(EXPORT          coroutine-config
        DESTINATION     ${CMAKE_INSTALL_PREFIX}/share/coroutine
)
