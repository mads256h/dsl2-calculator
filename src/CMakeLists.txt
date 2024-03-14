add_executable(test_thing test.cpp)
target_link_libraries(test_thing PRIVATE doctest::doctest_with_main)
add_test(NAME test_thing COMMAND test_thing)
