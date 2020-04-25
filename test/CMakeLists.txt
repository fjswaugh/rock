add_executable(rock_test
    main.cpp
    doctest_formatting.h
    test_move_recommend_speed.cpp
    test_move_gen_speed.cpp
    test_parse.cpp)

target_compile_options(rock PRIVATE ${ROCK_COMMON_FLAGS})

target_compile_features(rock_test PRIVATE cxx_std_17)
target_link_libraries(rock_test PRIVATE rock doctest::doctest)

add_test(NAME rock_test_main COMMAND rock_test)
