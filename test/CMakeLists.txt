FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/onqtam/doctest.git
    GIT_TAG        2.3.7)
FetchContent_MakeAvailable(doctest)

add_executable(rock_test
    example_boards.h
    doctest_formatting.h
    unit_testing/main.cpp
    unit_testing/test_move_generation.cpp
    unit_testing/test_parse.cpp)

target_include_directories(rock_test PRIVATE . $<TARGET_PROPERTY:rock,INCLUDE_DIRECTORIES>)
target_compile_options(rock_test PRIVATE ${ROCK_COMMON_FLAGS})
target_link_libraries(rock_test PRIVATE rock doctest::doctest)

add_executable(rock_perf_test
    example_boards.h
    doctest_formatting.h
    performance/common.h
    performance/main.cpp
    performance/count_moves.cpp
    performance/analyze_position.cpp)

target_include_directories(rock_perf_test PRIVATE . $<TARGET_PROPERTY:rock,INCLUDE_DIRECTORIES>)
target_compile_options(rock_perf_test PRIVATE ${ROCK_COMMON_FLAGS})
target_link_libraries(rock_perf_test PRIVATE rock doctest::doctest)

add_test(NAME rock_test_main COMMAND rock_test)
add_test(NAME rock_test_perf COMMAND rock_perf_test)
