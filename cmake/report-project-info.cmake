# 
#   Author  : github.com/luncliff (luncliff@gmail.com)
#
message(STATUS "========== ${PROJECT_NAME} : ${PROJECT_VERSION} ==========")

message(STATUS "System      \t: ${CMAKE_SYSTEM}")
message(STATUS "Build Type  \t: ${CMAKE_BUILD_TYPE}")

message(STATUS "Path")
message(STATUS "  Root      \t: ${CMAKE_SOURCE_DIR}")
message(STATUS "  Project   \t: ${PROJECT_SOURCE_DIR}")
message(STATUS "  Install   \t: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  Current   \t: ${CMAKE_CURRENT_SOURCE_DIR}")

message(STATUS "CMake")
message(STATUS "  Version   \t: ${CMAKE_VERSION}")
message(STATUS "  Toolchain \t: ${CMAKE_TOOLCHAIN_FILE}")

message(STATUS "Compiler")
message(STATUS "  ID        \t: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "  Version   \t: ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Path      \t: ${CMAKE_CXX_COMPILER}")

if(ANDROID)
message(STATUS "Android")
message(STATUS "  Platform\t: ${ANDROID_PLATFORM}")     # API level
message(STATUS "  Arch    \t: ${ANDROID_ARCH_NAME}")    # 
message(STATUS "  ABI     \t: ${ANDROID_ABI}")          # 
message(STATUS "  NDK Path\t: ${ANDROID_NDK}")          # Path/to/NDK
message(STATUS "  STL     \t: ${ANDROID_STL}")          # expect c++_shared
elseif(IOS)
# ...
endif()