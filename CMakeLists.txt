cmake_minimum_required(VERSION 3.14)

# set the project name
project(Genetic_Algorithm)

set(TEAM_NAME "Cuprite")
set(CMAKE_CXX_STANDARD 20)

# Build the Google Benchmark microbenchmarks. Off by default so CI and
# offline builds do not pay the FetchContent cost; the penalty-function
# benchmark target lives in tests/simulator_tests/bench_economics.cpp.
option(BUILD_BENCHMARKS "Build Google Benchmark targets" OFF)

if(BUILD_BENCHMARKS)
    include(FetchContent)
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(googlebench
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG        v1.9.1)
    FetchContent_MakeAvailable(googlebench)
endif()

# add graph library submodule
add_subdirectory(esegraph)

# set the include path
include_directories(include esegraph/include)

# add the main code
add_subdirectory(src)

# add the tests
include(CTest)
enable_testing()
add_subdirectory(tests)


