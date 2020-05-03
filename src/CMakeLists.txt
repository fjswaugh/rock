add_library(rock
    algorithms.cpp
    parse.cpp
    format.cpp
    game.cpp
    fen.cpp
    types.cpp
    internal/table_generation.h
    internal/bit_operations.h
    internal/transposition_table.h
    internal/diagnostics.h
    internal/internal_types.h
    internal/search.h
    internal/move_generation.h
    internal/evaluate.h
    ../include/rock/fen.h
    ../include/rock/game.h
    ../include/rock/algorithms.h
    ../include/rock/common.h
    ../include/rock/format.h
    ../include/rock/parse.h
    ../include/rock/starting_position.h
    ../include/rock/types.h)

target_compile_options(rock PRIVATE ${ROCK_COMMON_FLAGS})
target_compile_options(rock PRIVATE "$<$<CONFIG:Release>:${ROCK_RELEASE_FLAGS}>")

target_include_directories(rock PUBLIC ../include PRIVATE .)
target_compile_features(rock PUBLIC cxx_std_17)
target_link_libraries(rock PUBLIC fmt::fmt PRIVATE absl::hash absl::flat_hash_map)
