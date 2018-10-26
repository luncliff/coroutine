# 
#   Author  : github.com/luncliff (luncliff@gmail.com)
#
message(STATUS "========== ${PROJECT_NAME} : ${PROJECT_VERSION} ==========")
message(STATUS "Toolchain   \t: ${CMAKE_TOOLCHAIN_FILE}")
message(STATUS "Build Type  \t: ${CMAKE_BUILD_TYPE}")

message(STATUS "Path")
message(STATUS "  Root      \t: ${ROOT_DIR}")
message(STATUS "  Project   \t: ${PROJECT_DIR}")
message(STATUS "  Install   \t: ${CMAKE_INSTALL_PREFIX}")

message(STATUS "System      \t: ${CMAKE_SYSTEM}")
message(STATUS "Platform    \t: ${PLATFORM}")

message(STATUS "Compiler")
message(STATUS "  ID        \t: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "  Version   \t: ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Path      \t: ${CMAKE_CXX_COMPILER}")
