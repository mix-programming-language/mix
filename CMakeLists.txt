cmake_minimum_required(VERSION 3.14)
project(mix)

# ----- dependencies ----- #

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

if(NOT MIX_DEPS_DIR)
    get_filename_component(MIX_DEPS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps ABSOLUTE)
    message("-- `MIX_DEPS_DIR` is not set. set `MIX_DEPS_DIR` to `${MIX_DEPS_DIR}`.")
endif()

if(NOT TARGET cutils_static)
    set(CUTILS_BUILD_TESTS OFF CACHE BOOL "")
    set(CUTILS_DEPS_DIR ${MIX_DEPS_DIR})
    FetchContent_Declare(cutils
        GIT_REPOSITORY "https://github.com/ouonline/cutils.git"
        GIT_TAG master
        SOURCE_DIR ${MIX_DEPS_DIR}/cutils
        BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/cutils-build
        SUBBUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/cutils-subbuild)
    FetchContent_MakeAvailable(cutils)
endif()

if(NOT TARGET logger_static)
    set(LOGGER_BUILD_TESTS OFF CACHE BOOL "")
    set(LOGGER_DEPS_DIR ${MIX_DEPS_DIR})
    FetchContent_Declare(logger
        GIT_REPOSITORY "https://github.com/ouonline/logger.git"
        GIT_TAG master
        SOURCE_DIR ${MIX_DEPS_DIR}/logger
        BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/logger-build
        SUBBUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/logger-subbuild)
    FetchContent_MakeAvailable(logger)
endif()

# ----- targets ----- #

set(__ASAN_FLAGS__ "-fsanitize=undefined -fsanitize=address -fno-omit-frame-pointer")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${__ASAN_FLAGS__}")
unset(__ASAN_FLAGS__)

file(GLOB MIX_SRC ${CMAKE_CURRENT_SOURCE_DIR}/mix/*.c)
add_library(mix_static STATIC ${MIX_SRC})
target_include_directories(mix_static PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${xxhash_SOURCE_DIR})
target_compile_features(mix_static PUBLIC cxx_std_11)
target_link_libraries(mix_static PUBLIC cutils_static logger_static m)

# ----- tests ----- #

add_subdirectory(bison)
add_subdirectory(tests)
