#
#   Author  : github.com/luncliff (luncliff@gmail.com)
#
cmake_minimum_required(VERSION 3.8)

# generate_exe_test( ... )
function(generate_exe_test NAME)
    set(TEST_NAME test_${NAME})
    # create a test exe with the given name ...
    add_executable(${TEST_NAME} ${NAME}.cpp )
    add_test(${NAME} ${TEST_NAME})

    # ... example cmake configuration of the library user ...
    set_target_properties(${TEST_NAME}
    PROPERTIES
        CXX_STANDARD 17
    )
    target_include_directories(${TEST_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${GSL_INCLUDE_DIR}
    )
    target_link_libraries(${TEST_NAME}
    PUBLIC
        coroutine
    )
    if(WIN32)
        target_compile_definitions(${TEST_NAME}
        PRIVATE
            _CRT_SECURE_NO_WARNINGS
        )
    elseif(UNIX)
        # code coverage option lead to compiler crash
        # list(APPEND CMAKE_CXX_FLAGS "--coverage")
        target_compile_options(${TEST_NAME}
        PRIVATE
            -O0 # temporary disable optimization. 
                # will be removed after test code changes
            -g -Wall -Wextra -Wno-unknown-pragmas 
        )
    endif()
endfunction()

generate_exe_test( sequence_destroy )
generate_exe_test( sequence_frame_status )
generate_exe_test( sequence_suspend_await )
generate_exe_test( sequence_suspend_yield )
generate_exe_test( sequence_yield_never )
generate_exe_test( sequence_yield_once )
