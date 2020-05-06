add_executable(rock_test
    main.cpp
    doctest_formatting.h
    example_boards.h
    test_move_recommend_speed.cpp
    test_move_gen_speed.cpp
    test_parse.cpp)

target_compile_options(rock_test PRIVATE ${ROCK_COMMON_FLAGS})
target_link_libraries(rock_test PRIVATE rock doctest::doctest)
target_include_directories(rock_test PRIVATE $<TARGET_PROPERTY:rock,INCLUDE_DIRECTORIES>)

add_test(NAME rock_test_main COMMAND rock_test)

find_package(benchmark REQUIRED)

add_executable(rock_perf_test
    perf_test.cpp)

target_compile_options(rock_perf_test PRIVATE ${ROCK_COMMON_FLAGS})
target_link_libraries(rock_perf_test PRIVATE rock benchmark::benchmark)

