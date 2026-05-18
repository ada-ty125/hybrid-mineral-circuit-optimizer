cmake_minimum_required(VERSION 3.10)

# set the project name
project(Genetic_Algorithm)

set(TEAM_NAME "default")
set(CMAKE_CXX_STANDARD 20)

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


