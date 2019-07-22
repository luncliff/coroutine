#
#   Author  : github.com/luncliff (luncliff@gmail.com)
#
cmake_minimum_required(VERSION 3.8)

unset(CMAKE_C_FLAGS)
unset(CMAKE_C_FLAGS_DEBUG)
unset(CMAKE_C_FLAGS_RELEASE)
unset(CMAKE_C_FLAGS_RELWITHDEBINFO)

unset(CMAKE_CXX_FLAGS)
unset(CMAKE_CXX_FLAGS_DEBUG)
unset(CMAKE_CXX_FLAGS_RELEASE)
unset(CMAKE_CXX_FLAGS_RELWITHDEBINFO)

# generate_exe_test( ... )
function(generate_exe_test NAME)
    set(TEST_NAME test_${NAME})
    # create a test exe with the given name ...
    add_executable(${TEST_NAME} ${NAME}.cpp)
    add_test(${NAME} ${TEST_NAME})

    # ... example cmake configuration of the library user ...
    set_target_properties(${TEST_NAME}
    PROPERTIES
        CXX_STANDARD 17
    )
    target_include_directories(${TEST_NAME}
    PRIVATE
        ${PROJECT_SOURCE_DIR}/interface
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${GSL_INCLUDE_DIR}
    )
    # target_link_libraries(${TEST_NAME}
    # PUBLIC
    #     coroutine ${ARGV1}
    # )
    target_compile_definitions(${TEST_NAME}
    PRIVATE
        CMAKE_TEST
    )
    if(WIN32)
        target_compile_definitions(${TEST_NAME}
        PRIVATE
            _CRT_SECURE_NO_WARNINGS
        )
    elseif(UNIX)
        if(${CMAKE_CXX_COMPILER_ID} MATCHES Clang)
            target_compile_options(${TEST_NAME}
            PUBLIC
                -std=c++2a -stdlib=libc++ -fcoroutines-ts 
                -fPIC
            PRIVATE
                -Wall -Wno-unknown-pragmas -Wno-unused-private-field
                -fno-rtti -fvisibility=hidden -ferror-limit=5
            )
        elseif(${CMAKE_CXX_COMPILER_ID} MATCHES GNU)
            target_compile_options(${TEST_NAME}
            PUBLIC
                -std=gnu++2a -fcoroutines
                -fPIC
                -fno-exceptions
                -fconcepts
            PRIVATE
                -Wall -Wno-unknown-pragmas
                -fno-rtti -fvisibility=hidden
            )
            target_link_libraries(${TEST_NAME}
            PUBLIC
                stdc++
            )
        endif()
    endif()
endfunction()

if(WIN32)


elseif(UNIX)

# https://github.com/iains/gcc-cxx-coroutines/tree/c%2B%2B-coroutines/gcc/testsuite/g%2B%2B.dg/coroutines
# docker run -it -d --name gcc_10_builder -w /code/build -v $(pwd):/code luncliff/gcc
generate_exe_test(  gcc_support_version )
generate_exe_test(  gcc_concepts_awaitable )
generate_exe_test(  gcc_concepts_promise )
generate_exe_test(  gcc_empty_handle )
generate_exe_test(  gcc_save_current_frame )
generate_exe_test(  gcc_assign_reference )

endif()
