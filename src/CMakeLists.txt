add_library(rock
    algorithms.cpp
    parse.cpp
    format.cpp
    table_generation.h
    ../include/rock/algorithms.h
    ../include/rock/common.h
    ../include/rock/format.h
    ../include/rock/parse.h
    ../include/rock/starting_board.h
    ../include/rock/types.h)

target_include_directories(rock PUBLIC ../include PRIVATE .)
target_compile_features(rock PUBLIC cxx_std_17)
target_link_libraries(rock PUBLIC fmt::fmt)
