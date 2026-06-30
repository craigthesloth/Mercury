add_executable(mercury_tests
    main_test.cpp
    test_threadpool.cpp
    test_taskcontroller.cpp
    test_signal.cpp
    test_coroutines.cpp
    test_stress.cpp
 "catch_amalgamated.cpp")

target_include_directories(mercury_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(mercury_tests PRIVATE cxx_std_20)

find_package(Threads REQUIRED)
target_link_libraries(mercury_tests PRIVATE Threads::Threads)

add_test(NAME MercuryTests COMMAND mercury_tests)