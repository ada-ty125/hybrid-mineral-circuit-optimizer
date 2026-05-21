set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find OpenMP package required for multithreaded evaluation and reproduction
find_package(OpenMP REQUIRED)

# add the genetic algorithm library (change sections as needed)
add_library(geneticAlgorithm Genetic_Algorithm.cpp PRNG.cpp)

# Enable OpenMP flags for the genetic algorithm library target
target_link_libraries(geneticAlgorithm PRIVATE OpenMP::OpenMP_CXX)

set_target_properties( geneticAlgorithm
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

# Economics on its own so test_economics builds without waiting on CUnit fixes.

add_library(circuitEconomics Economics.cpp)
set_target_properties( circuitEconomics
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

# build the circuit simulator as a testable library

add_library(circuitSimulator CUnit.cpp CCircuit.cpp CSimulator.cpp Visualization.cpp)
target_link_libraries(circuitSimulator PUBLIC circuitEconomics)
target_compile_definitions(circuitSimulator PRIVATE
    CIRCUIT_PROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
set_target_properties( circuitSimulator
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

# build the executable
add_executable(Circuit_Optimizer main.cpp)

# Link extra libraries and ensure OpenMP runtime is explicitly linked to the executable
target_link_libraries(Circuit_Optimizer PUBLIC circuitSimulator esegraph geneticAlgorithm OpenMP::OpenMP_CXX)

set_target_properties( Circuit_Optimizer
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
