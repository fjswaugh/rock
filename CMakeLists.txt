cmake_minimum_required(VERSION 3.11)

project(rock
    VERSION 0.0.0
    DESCRIPTION "Romanian checkers AI"
    LANGUAGES CXX)

set(MASTER_PROJECT OFF)
if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set(MASTER_PROJECT ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    set(CMAKE_EXPORT_COMPILE_COMMANDS "ON")
endif ()

option(ROCK_TEST "Generate tests" ${MASTER_PROJECT})
option(ROCK_RECOMMENDED_OPT "Use recommended optimization flags" ON)

####################################################################################################
# Compiler flags to be appended to target
####################################################################################################

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(WARNING_FLAGS /W4)
else ()
    set(WARNING_FLAGS -Wall -Wextra)
    if (ROCK_RECOMMENDED_OPT)
        set(MY_RELEASE_FLAGS -O3 -march=native)
    endif ()
endif ()

# Add no-semantic-interposition to gcc otherwise rock will be very slow when built with PIC
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    if (ROCK_RECOMMENDED_OPT)
        set(MY_RELEASE_FLAGS ${MY_RELEASE_FLAGS} -fno-semantic-interposition)
    endif()
endif()

####################################################################################################
# Dependencies
####################################################################################################

include(FetchContent)

if (${CMAKE_VERSION} VERSION_LESS 3.14)
    macro(FetchContent_MakeAvailable NAME)
        FetchContent_GetProperties(${NAME})
        if (NOT ${NAME}_POPULATED)
            FetchContent_Populate(${NAME})
            add_subdirectory(${${NAME}_SOURCE_DIR} ${${NAME}_BINARY_DIR})
        endif()
    endmacro()
endif()

FetchContent_Declare(
    fmtlib
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG        5.3.0)
FetchContent_MakeAvailable(fmtlib)

####################################################################################################
# Subdirectories
####################################################################################################

add_subdirectory(src)

if (ROCK_TEST)
    include(CTest)
    enable_testing()
    add_subdirectory(test)
endif()
