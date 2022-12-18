cmake_minimum_required(VERSION 3.14)
project(mix-tools)

add_executable(mix ${CMAKE_CURRENT_LIST_DIR}/mix.c)
target_link_libraries(mix PRIVATE mix_static)
target_compile_features(mix PRIVATE cxx_std_11)
