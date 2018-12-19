# ---------------------------------------------------------------------------
#
#   Author  : github.com/luncliff (luncliff@gmail.com)
#
# ---------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.8)

add_library(${PROJECT_NAME} # -> coroutine.lib
    ${PLATFORM}/dllmain.cpp
)

set_target_properties(${PROJECT_NAME}
PROPERTIES 
    CXX_STANDARD    17
)

target_include_directories(${PROJECT_NAME}
PUBLIC
    ${PROJECT_DIR}/interface
PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# target_link_libraries(${PROJECT_NAME}
# PUBLIC
#     GSL
# )

# to prevent creating sub-library,
#   incrementally attach code/properties using CMake include
include(thread/get-code.cmake)
include(${PLATFORM}/get-code.cmake)

# simply install header & binary
install(DIRECTORY       ${PROJECT_DIR}/interface/coroutine
        DESTINATION     ${CMAKE_INSTALL_PREFIX}/include
)
install(TARGETS         ${PROJECT_NAME}
        DESTINATION     ${CMAKE_INSTALL_PREFIX}/libs/${PLATFORM}
)

# library export
# ...
