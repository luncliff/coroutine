#
#   Author  : github.com/luncliff (luncliff@gmail.com)
#   Note
#       Try to include GSL.
#       This file is separated to make Root CMakeLists short
#
if(${CMAKE_TOOLCHAIN_FILE} MATCHES vcpkg.cmake)
    # portfile in vcpkg must give the path
    set(GSL_INCLUDE_DIR	${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include)
else()
    # use submodule
    add_subdirectory(external/ms-gsl)
    set(GSL_INCLUDE_DIR	${CMAKE_CURRENT_SOURCE_DIR}/external/ms-gsl/include)
endif()
if(NOT DEFINED GSL_INCLUDE_DIR)
    message(FATAL_ERROR "Path - GSL_INCLUDE_DIR is required")
else()
    message(STATUS "GSL (C++ Core Guideline Support Library)")
    message(STATUS "  Path      \t: ${GSL_INCLUDE_DIR}")
    message(STATUS)
endif()
