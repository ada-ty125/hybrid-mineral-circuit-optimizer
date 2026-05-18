cmake_minimum_required(VERSION 3.10)

project(RandomNumbers)

set(CMAKE_CXX_STANDARD 20)
find_package(OpenMP REQUIRED)
find_package(MPI REQUIRED)

include_directories(${MPI_INCLUDE_PATH})

add_executable(basic_rand basic_rand.cpp)
add_executable(slow_random slow_random.cpp)
add_executable(race_random race_random.cpp)
add_executable(fast_random1 fast_random1.cpp)
add_executable(fast_random2 fast_random2.cpp)

target_link_libraries(basic_rand PUBLIC OpenMP::OpenMP_CXX)
target_link_libraries(slow_random PUBLIC OpenMP::OpenMP_CXX)
target_link_libraries(race_random PUBLIC OpenMP::OpenMP_CXX)
target_link_libraries(fast_random1 PUBLIC OpenMP::OpenMP_CXX)
target_link_libraries(fast_random2 PUBLIC OpenMP::OpenMP_CXX)


target_link_libraries(basic_rand PUBLIC MPI::MPI_CXX)
target_link_libraries(slow_random PUBLIC MPI::MPI_CXX)
target_link_libraries(race_random PUBLIC MPI::MPI_CXX)
target_link_libraries(fast_random1 PUBLIC MPI::MPI_CXX)
target_link_libraries(fast_random2 PUBLIC MPI::MPI_CXX)
