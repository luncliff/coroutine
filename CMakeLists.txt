#
#   Author  : github.com/luncliff (luncliff@gmail.com)
#   Note
#       CMake support for non-VisualStudio projects
#
#   Support
#       - MSVC  + Windows   (Visual Studio)
#       - Clang + Windows   (Ninja)
#       - Clang + MacOS     (Unix Makefiles)
#       - Clang + Linux     (Unix Makefiles. WSL, Ubuntu 1604 and later)
#
#   Working
#       - GCC   + Linux     (Ninja, Unix Makefiles)
#
cmake_minimum_required(VERSION 3.8)

project(coroutine 
        LANGUAGES CXX 
        VERSION 1.5.0
)

# import cmake code snippets. see `cmake/`
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
include(check-compiler)
include(report-project-info)

if(${CMAKE_TOOLCHAIN_FILE} MATCHES vcpkg.cmake)
    # portfile in vcpkg must give the path
    set(GSL_INCLUDE_DIR	${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include)
else()
    # use submodule
    add_subdirectory(external/guideline)
	set(GSL_INCLUDE_DIR	${CMAKE_CURRENT_SOURCE_DIR}/external/guideline/include)
endif()

set(INTERFACE_DIR ${PROJECT_SOURCE_DIR}/interface)
add_subdirectory(modules/portable)
add_subdirectory(modules/concrt)

# Export/Install for find_package()
#   - https://cmake.org/cmake/help/latest/manual/cmake-packages.7.html
#   - https://cmake.org/cmake/help/latest/command/export.html
#   - https://cmake.org/cmake/help/latest/command/install.html
#   - https://gitlab.kitware.com/cmake/community/wikis/doc/tutorials/Exporting-and-Importing-Targets#exporting-from-an-installation-tree

# export declared cmake targets
# 'coroutine-targets' is a better name, but we will merge their contents 
# into the file 'coroutine-config.cmake' without using 'configure_file()' which requires config template
install(EXPORT      coroutine-config
        NAMESPACE   coro::
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/coroutine
)

# generate/install config & version info
include(CMakePackageConfigHelpers)
set(VERSION_FILE_PATH   ${CMAKE_BINARY_DIR}/cmake/coroutine-config-version.cmake)
write_basic_package_version_file(${VERSION_FILE_PATH}
    VERSION             ${PROJECT_VERSION}
    COMPATIBILITY       SameMajorVersion
)
install(FILES           ${VERSION_FILE_PATH} 
        DESTINATION     ${CMAKE_INSTALL_PREFIX}/share/coroutine
)

# Test projects
if(TEST_DISABLED)
    message(STATUS "Test is disabled.")
    return()
elseif(NOT ${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
    message(STATUS "This is not a root project. Skipping the tests")
    return()
elseif(NO_TESTS)
    message(STATUS "'NO_TESTS' is OFF. Skipping the tests")
    return()
endif()

enable_testing() # ctest
add_subdirectory(test)
