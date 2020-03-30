add_executable(rock_demo
    main.cpp)

target_compile_features(rock_demo PRIVATE cxx_std_17)
target_link_libraries(rock_demo PRIVATE rock)

pybind11_add_module(rock_python
    python_bindings.cpp)
set_target_properties(rock_python PROPERTIES OUTPUT_NAME rock)
target_link_libraries(rock_python PRIVATE rock)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    target_compile_options(rock_python PUBLIC -fsized-deallocation)
endif()
