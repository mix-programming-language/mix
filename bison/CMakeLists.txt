cmake_minimum_required(VERSION 3.14)
project(mix-bison)

execute_process(COMMAND bison -d ${CMAKE_CURRENT_SOURCE_DIR}/mix.y -Wcounterexamples)

add_executable(a.out ${CMAKE_CURRENT_BINARY_DIR}/../mix.tab.c)
target_link_libraries(a.out PRIVATE mix_static)
target_compile_features(a.out PRIVATE cxx_std_11)
