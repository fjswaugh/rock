cmake_minimum_required(VERSION 3.11)

project(rock
    VERSION 0.0.0
    DESCRIPTION "Romanian checkers AI"
    LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS "ON")

if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set(CMAKE_CXX_EXTENSIONS OFF)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

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

# TODO
# For now, we set this to true so that the fmt library works with pybind11
# Unclear on the reasons, but will investigate further
set(BUILD_SHARED_LIBS TRUE)

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GCC")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fno-semantic-interposition")
endif()

FetchContent_Declare(
    fmtlib
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG        5.3.0)
FetchContent_MakeAvailable(fmtlib)

FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    GIT_TAG        v2.4.3)
FetchContent_MakeAvailable(pybind11)

add_subdirectory(src)
add_subdirectory(apps)

include(CTest)
add_subdirectory(test)
