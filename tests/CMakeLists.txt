
find_package(GTest REQUIRED)
include(GoogleTest)

# include(FetchContent)
# FetchContent_Declare(
# googletest
# GIT_REPOSITORY https://github.com/google/googletest.git
# GIT_TAG        release-1.11.0
# )
# For Windows: Prevent overriding the parent project's compiler/linker settings
# set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
# FetchContent_MakeAvailable(googletest)
MESSAGE(STATUS "googletest_POPULATED: " ${googletest_POPULATED})
MESSAGE(STATUS "googletest_SOURCE_DIR: " ${googletest_SOURCE_DIR})
MESSAGE(STATUS "googletest_BINARY_DIR: " ${googletest_BINARY_DIR})
MESSAGE(STATUS "GTest_FOUND: " ${GTest_FOUND})
MESSAGE(STATUS "GTest_DIR: " ${GTest_DIR})

set(TEST_BASIC "test_basic")
set(TEST_HEAVY "test_heavy")
set(TEST_WAIT_FOR_TIMEOUT "test_wait_for_timeout")
set(TEST_WAIT_FOR_NO_TIMEOUT "test_wait_for_no_timeout")
set(TEST_RETURN_VALUE "test_return_value")
set(TEST_RETURN_EXCEPTION "test_return_exception")
set(TEST_REFERENCE_PASSING "test_reference_passing")

set(TEST_G_BASIC "test_basic_2")
set(TEST_G_EXCEPTION "test_return_exception2")

list(APPEND tests
    ${TEST_BASIC}
    ${TEST_HEAVY}
    ${TEST_RETURN_VALUE}
    ${TEST_RETURN_EXCEPTION}
    ${TEST_WAIT_FOR_TIMEOUT}
    ${TEST_WAIT_FOR_NO_TIMEOUT}
    ${TEST_REFERENCE_PASSING}
)

foreach(testname IN LISTS tests)
    # Executable
    add_executable(${testname} "${testname}.cpp")

    set_target_properties(${testname} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out/test
        RUNTIME_OUTPUT_NAME ${testname}
    )

    target_link_libraries(${testname}
        # PRIVATE ThreadPool::ThreadPool
        PRIVATE ${THREADPOOL}
    )

    add_test(NAME ${testname} COMMAND $<TARGET_FILE:${testname}>)
endforeach()

list(APPEND gtests
    ${TEST_G_BASIC}
    ${TEST_G_EXCEPTION}
)

foreach(testname IN LISTS gtests)
    add_executable(${testname} "${testname}.cpp")
    set_target_properties(${testname} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out/test
        RUNTIME_OUTPUT_NAME ${testname}
    )

    target_link_libraries(${testname}
        PRIVATE ${THREADPOOL}
        PRIVATE GTest::gtest_main
    )

    # add_test(NAME ${TEST_G_BASIC} COMMAND $<TARGET_FILE:${TEST_G_BASIC}>)
    gtest_discover_tests(${testname})
endforeach()

# Coverage
if(ENABLE_COVERAGE)
    set(COVERAGE_MAIN "${PROJECT}_coverage")
    set(COVERAGE_LCOV_EXCLUDES
        "${PROJECT_SOURCE_DIR}/cmake/*"
        "${PROJECT_SOURCE_DIR}/tests/*"
        "${PROJECT_SOURCE_DIR}/build/*"
        "C:/msys64/mingw64/include/*"
        "C:/msys64/mingw64/lib/gcc/x86_64-w64-mingw32/12.2.0/include")

    setup_target_for_coverage_lcov(
        NAME ${COVERAGE_MAIN}
        EXECUTABLE ctest
        DEPENDENCIES ${TEST_BASIC})
endif()