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
)

target_link_libraries(${PROJECT_NAME}
PUBLIC
    GSL
)

# to prevent creating sub-library,
#   incrementally attach code/properties using CMake include
list(APPEND CMAKE_MODULE_PATH
            ${CMAKE_CURRENT_SOURCE_DIR}
)
include(${SYSTEM_NAME})

# simply install header & binary
install(DIRECTORY       ${PROJECT_SOURCE_DIR}/interface/coroutine
        DESTINATION     ${CMAKE_INSTALL_PREFIX}/include
)
install(TARGETS         ${PROJECT_NAME}
        DESTINATION     ${CMAKE_INSTALL_PREFIX}/libs/${PLATFORM}
)

# library export
# ...
