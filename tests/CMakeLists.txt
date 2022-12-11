cmake_minimum_required(VERSION 3.10)
project(mix-test)

add_executable(test_lex src/test_lex.c)
target_link_libraries(test_lex PRIVATE mix_static)
target_compile_features(test_lex PRIVATE cxx_std_11)
target_compile_definitions(test_lex PRIVATE
    MIX_TESTDATA_DIR="${CMAKE_CURRENT_LIST_DIR}/testdata")
