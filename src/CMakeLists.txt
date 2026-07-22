set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# AppleClang ships without an OpenMP runtime. Discover a Homebrew libomp installation
# automatically so a clean macOS checkout does not depend on a pre-populated CMake cache.
if(APPLE AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    find_path(HOMEBREW_LIBOMP_INCLUDE_DIR omp.h
        HINTS /opt/homebrew/opt/libomp/include /usr/local/opt/libomp/include)
    find_library(HOMEBREW_LIBOMP_LIBRARY omp
        HINTS /opt/homebrew/opt/libomp/lib /usr/local/opt/libomp/lib
        NO_DEFAULT_PATH)

    if(HOMEBREW_LIBOMP_INCLUDE_DIR AND HOMEBREW_LIBOMP_LIBRARY)
        set(OpenMP_CXX_FLAGS "-Xclang -fopenmp" CACHE STRING
            "CXX compiler flags for OpenMP parallelization" FORCE)
        set(OpenMP_CXX_LIB_NAMES "libomp" CACHE STRING
            "CXX compiler libraries for OpenMP parallelization" FORCE)
        set(OpenMP_libomp_LIBRARY "${HOMEBREW_LIBOMP_LIBRARY}" CACHE FILEPATH
            "Path to the Homebrew OpenMP runtime" FORCE)
        set(OpenMP_CXX_INCLUDE_DIR "${HOMEBREW_LIBOMP_INCLUDE_DIR}" CACHE PATH
            "Path to the Homebrew OpenMP headers" FORCE)
    endif()
endif()

# Find OpenMP package required for multithreaded evaluation and reproduction.
find_package(OpenMP REQUIRED COMPONENTS CXX)

# add the genetic algorithm library (change sections as needed)
set(GA_SOURCE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/Genetic_Algorithm.cpp" CACHE FILEPATH
    "Genetic algorithm implementation source file")
get_filename_component(GA_SOURCE_FILE "${GA_SOURCE_FILE}" ABSOLUTE BASE_DIR "${CMAKE_SOURCE_DIR}")
message(STATUS "Using genetic algorithm source: ${GA_SOURCE_FILE}")
add_library(geneticAlgorithm "${GA_SOURCE_FILE}" PRNG.cpp)

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
add_executable(Circuit_Optimizer main.cpp Circuit_Optimization_Problem.cpp)

# Link extra libraries and ensure OpenMP runtime is explicitly linked to the executable
target_link_libraries(Circuit_Optimizer PUBLIC circuitSimulator esegraph geneticAlgorithm OpenMP::OpenMP_CXX)

set_target_properties( Circuit_Optimizer
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_executable(Parameter_Tuning Parameter_Tuning.cpp Circuit_Optimization_Problem.cpp)
target_link_libraries(Parameter_Tuning PUBLIC circuitSimulator esegraph geneticAlgorithm OpenMP::OpenMP_CXX)

set_target_properties( Parameter_Tuning
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
