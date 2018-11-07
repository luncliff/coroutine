#
#   Author  : github.com/luncliff (luncliff@gmail.com)
#
set(ROOT_DIR    ${CMAKE_SOURCE_DIR}         )
set(PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR} )

if(ANDROID)
    set(PLATFORM ${ANDROID_ABI})
elseif(IOS OR IOS_DEPLOYMENT_TARGET)
    set(PLATFORM iphone)
elseif(${CMAKE_SYSTEM} MATCHES Windows)
    set(PLATFORM windows)
elseif(${CMAKE_SYSTEM} MATCHES Darwin)
    set(OSX true)
    set(PLATFORM posix)
elseif(${CMAKE_SYSTEM} MATCHES Linux)
    set(LINUX true)
    set(PLATFORM posix)
endif()
# end