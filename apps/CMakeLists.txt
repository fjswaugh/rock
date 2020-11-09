FetchContent_Declare(
    cxxopts
    GIT_REPOSITORY https://github.com/jarro2783/cxxopts
    GIT_TAG        v2.2.0)
FetchContent_MakeAvailable(cxxopts)

add_executable(rock_analyze_position
    analyze_position.cpp)

target_compile_options(rock_analyze_position PRIVATE ${ROCK_COMMON_FLAGS})
target_link_libraries(rock_analyze_position PRIVATE rock cxxopts)

add_executable(rock_play_against_other
    play_against_other.cpp)

target_compile_options(rock_play_against_other PRIVATE ${ROCK_COMMON_FLAGS})
target_link_libraries(rock_play_against_other PRIVATE rock cxxopts)
