# Based on:
# https://cmake.org/cmake/help/latest/manual/cmake-packages.7.html
# https://cmake.org/cmake/help/latest/guide/tutorial/Adding%20Export%20Configuration.html

add_library(${THREADPOOL} INTERFACE "ThreadPool-CPP.h")

if(${THREADPOOL_DEBUG} STREQUAL "ON")
    # add -DTHREADPOOL_DEBUG compile definitions in debug mode only
    target_compile_definitions(${THREADPOOL} INTERFACE "$<$<CONFIG:Debug>:THREADPOOL_DEBUG>")
endif()

target_link_libraries(${THREADPOOL}
    INTERFACE $<BUILD_INTERFACE:compiler_warnings>
    INTERFACE $<BUILD_INTERFACE:pthread>
)

# https://www.foonathan.net/2018/10/cmake-warnings/
# The interface properties are only given to external targets linking to your target,
# and never used when compiling the target itself.
# SYSTEM: do not show compliler warning for targets linked to this Interface lib
target_include_directories(${THREADPOOL} SYSTEM INTERFACE
    $<INSTALL_INTERFACE:include/${THREADPOOL}>
)

target_include_directories(${THREADPOOL} INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include> # cmake will prepend CMAKE_INSTALL_PREFIX
)

# #include(GenerateExportHeader)
# #generate_export_header(${THREADPOOL})
install(TARGETS ${THREADPOOL} compiler_warnings
    EXPORT ThreadPoolTargets
    DESTINATION lib/${THREADPOOL}
)
install(FILES
    ${PROJECT_SOURCE_DIR}/src/ThreadPool-CPP.h
    # ${CMAKE_CURRENT_BINARY_DIR}/ThreadPool-CPP_export.h
    DESTINATION include/${THREADPOOL}
)