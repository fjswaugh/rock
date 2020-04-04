FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/onqtam/doctest.git
    GIT_TAG        2.3.7)
FetchContent_MakeAvailable(doctest)

add_executable(rock_test
    main.cpp
    test_move_gen_speed.cpp
    test_parse.cpp)

target_compile_features(rock_test PRIVATE cxx_std_17)
target_link_libraries(rock_test PRIVATE rock doctest::doctest)

add_test(NAME rock_test_main COMMAND rock_test)
