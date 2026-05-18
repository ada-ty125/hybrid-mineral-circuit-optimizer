#Set the lowest version of cmake that can be used
cmake_minimum_required(VERSION 3.10)

# set the project name (and the name f)
project(MPIHelloWorld)

# add the executable
add_executable(mpi_example main.cpp)

find_package(MPI REQUIRED)

include_directories(${MPI_CXX_INCLUDE_PATH})
target_link_libraries(mpi_example PUBLIC MPI::MPI_CXX)