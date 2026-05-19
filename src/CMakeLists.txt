# add the genetic algorithm library (change sectionsas needed)

add_library(geneticAlgorithm Genetic_Algorithm.cpp PRNG.cpp)

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
set_target_properties( circuitSimulator
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

# build the executable

add_executable(Circuit_Optimizer main.cpp)

# add extra libraries here if you need to
target_link_libraries(Circuit_Optimizer PUBLIC circuitSimulator esegraph geneticAlgorithm)

set_target_properties( Circuit_Optimizer
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
