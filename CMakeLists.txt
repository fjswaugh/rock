cmake_minimum_required(VERSION 3.14)

project(rock
    VERSION 0.0.1
    DESCRIPTION "Romanian checkers AI"
    LANGUAGES CXX)

set(MASTER_PROJECT OFF)
if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set(MASTER_PROJECT ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    set(CMAKE_EXPORT_COMPILE_COMMANDS "ON")
endif ()

option(ROCK_TEST                    "Generate tests"                                ${MASTER_PROJECT})
option(ROCK_RECOMMENDED_RELEASE_OPT "Use recommended optimization flags in Release" ON)
option(ROCK_ARCHITECTURE_OPT        "Use architecture-specific optimizations"       OFF)

####################################################################################################
# Compiler flags to be appended to target
####################################################################################################

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    string(REGEX REPLACE "/W[0-9]" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    set(ROCK_COMMON_FLAGS /W4)
else ()
    set(ROCK_COMMON_FLAGS -Wall -Wextra -fvisibility=hidden)
    if (ROCK_RECOMMENDED_RELEASE_OPT)
        set(ROCK_RELEASE_FLAGS ${ROCK_RELEASE_FLAGS} -O3)
    endif ()
    if (ROCK_ARCHITECTURE_OPT)
        set(ROCK_RELEASE_FLAGS ${ROCK_RELEASE_FLAGS} -march=native)
    endif ()
endif ()

# Add no-semantic-interposition to gcc otherwise rock will be very slow when
# built as a shared library
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    if (ROCK_RECOMMENDED_RELEASE_OPT)
        set(ROCK_RELEASE_FLAGS ${ROCK_RELEASE_FLAGS} -fno-semantic-interposition)
    endif()
endif()

####################################################################################################
# Dependencies
####################################################################################################

include(FetchContent)

FetchContent_Declare(
    fmtlib
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG        5.3.0)
FetchContent_MakeAvailable(fmtlib)

####################################################################################################
# Subdirectories
####################################################################################################

add_subdirectory(src)
add_subdirectory(apps)

if (ROCK_TEST)
    include(CTest)
    enable_testing()
    add_subdirectory(test)
endif()
