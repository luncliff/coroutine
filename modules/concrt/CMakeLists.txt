
add_library(concrt
    libmain.cpp
)
set_target_properties(concrt
PROPERTIES
    CXX_STANDARD    17
)
target_include_directories(concrt
PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(concrt
PUBLIC
    portable
PRIVATE
    ${CMAKE_DL_LIBS}
)

if(NOT BUILD_SHARED_LIBS)
    message(WARNING "forcing static build")
    target_compile_definitions(concrt
    PUBLIC
        FORCE_STATIC_LINK
    )
endif()

if(WIN32)
    target_compile_definitions(concrt
    PRIVATE
        WIN32_LEAN_AND_MEAN NOMINMAX
    )
endif()

install(FILES   ${PROJECT_SOURCE_DIR}/include/concrt.h
        DESTINATION ${CMAKE_INSTALL_PREFIX}/include
)
install(TARGETS         concrt
        EXPORT          coroutine-config
        RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
        LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
        ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
)
