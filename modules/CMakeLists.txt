# ---------------------------------------------------------------------------
#
#   Author  : github.com/luncliff (luncliff@gmail.com)
#
# ---------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.8)

string(TOLOWER ${CMAKE_SYSTEM_NAME} SYSTEM_NAME)

add_library(${PROJECT_NAME} # -> coroutine.lib
    ${SYSTEM_NAME}/dllmain.cpp
)
add_library(coro ALIAS ${PROJECT_NAME})

set_target_properties(${PROJECT_NAME}
PROPERTIES
    CXX_STANDARD    17
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

# to prevent creating sub-library,
#   incrementally attach code/properties using CMake include
list(APPEND CMAKE_MODULE_PATH
            ${CMAKE_CURRENT_SOURCE_DIR}
)
include(${SYSTEM_NAME})

# library export

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
install(TARGETS         ${PROJECT_NAME}
        EXPORT          coroutine-config    # --> config for find_package
        RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
        LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
        ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
)
install(EXPORT          coroutine-config
        DESTINATION     ${CMAKE_INSTALL_PREFIX}/share/coroutine
)
