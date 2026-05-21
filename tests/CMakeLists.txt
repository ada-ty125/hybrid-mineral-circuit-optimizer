project(tests)

function(ADD_TESTS TESTS LABELS)
    foreach(TEST IN LISTS TESTS)
        add_executable(${TEST} ${TEST}.cpp)
        target_link_libraries(${TEST} ${ARGN})
        target_include_directories(${TEST} PRIVATE ${PROJECT_SOURCE_DIR}/includes)
        set_target_properties(${TEST} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/bin")
        add_test(NAME ${TEST} COMMAND ${TEST})
        set_property(TEST ${TEST} PROPERTY LABELS ${LABELS})
    endforeach()
endfunction()

add_subdirectory(simulator_tests)
add_subdirectory(ga_tests)

add_executable(speed_checker speed_checker.cpp)
target_link_libraries(speed_checker circuitSimulator esegraph)
target_include_directories(speed_checker PRIVATE ../includes)
set_target_properties(speed_checker PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/bin")

make_directory(${CMAKE_BINARY_DIR}/tests/bin/data)
add_custom_target(copy_data
COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/tests/data ${CMAKE_BINARY_DIR}/tests/bin/data
)

add_test(NAME speed_checker
         COMMAND speed_checker 1000
         WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/tests/bin")
set_property(TEST speed_checker PROPERTY LABELS simulator)
set_property(TEST speed_checker PROPERTY TIMEOUT 10)
add_dependencies(speed_checker copy_data)


add_executable(post_processing_test post_processing_test.cpp)
target_link_libraries(post_processing_test circuitSimulator esegraph)
target_include_directories(post_processing_test PRIVATE ../include)
set_target_properties(post_processing_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/bin")
add_test(NAME post_processing_test COMMAND post_processing_test
         WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/tests/bin")
set_property(TEST post_processing_test PROPERTY LABELS post_processing)


set(CUPRITE_EXECUTABLE_TEST_THREADS "4" CACHE STRING
    "OpenMP threads used by the Circuit_Optimizer executable test")
add_test(NAME executable
         COMMAND ${CMAKE_COMMAND} -E env
             OMP_NUM_THREADS=${CUPRITE_EXECUTABLE_TEST_THREADS}
             OMP_DYNAMIC=FALSE
             $<TARGET_FILE:Circuit_Optimizer>
             --population-size 16
             --max-iterations 3
             --crossover-probability 0.935
             --mutation-probability 0.214
             --tournament-size 2
             --early-stop-patience 3
             --num-crossover-points 1
             --elite-count 1
             --mode fixed
             --output "${CMAKE_BINARY_DIR}/tests/bin/executable_smoke"
             --output-format dot)
set_property(TEST executable PROPERTY PROCESSORS ${CUPRITE_EXECUTABLE_TEST_THREADS})

# test_economics needs the reference CSV available at runtime under data/
# relative to its working directory.
add_dependencies(test_economics copy_data)

# Google Benchmark target for the penalty function. Built only when
# -DBUILD_BENCHMARKS=ON; otherwise FetchContent is skipped and this block
# is a no-op.
if(BUILD_BENCHMARKS)
    # Best-effort capture of the current git sha so each CSV row is
    # traceable to a commit. Falls back to "unknown" outside a git
    # checkout.
    execute_process(
        COMMAND git rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE CUPRITE_GIT_SHA_VALUE
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(NOT CUPRITE_GIT_SHA_VALUE)
        set(CUPRITE_GIT_SHA_VALUE "unknown")
    endif()

    set(CUPRITE_BENCH_OUTPUT_DIR_VALUE
        "${CMAKE_BINARY_DIR}/tests/bin/benchmarks")
    file(MAKE_DIRECTORY "${CUPRITE_BENCH_OUTPUT_DIR_VALUE}")

    add_executable(bench_economics simulator_tests/bench_economics.cpp)
    target_link_libraries(bench_economics PRIVATE circuitEconomics
        benchmark::benchmark)
    target_compile_definitions(bench_economics PRIVATE
        CUPRITE_GIT_SHA="${CUPRITE_GIT_SHA_VALUE}"
        CUPRITE_BENCH_OUTPUT_DIR="${CUPRITE_BENCH_OUTPUT_DIR_VALUE}")
    set_target_properties(bench_economics PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/bin/benchmarks")
    add_dependencies(bench_economics copy_data)

    # Copy data/ next to the bench binary too, so the reference CSV is
    # readable when running directly from tests/bin/benchmarks/.
    add_custom_target(copy_data_bench
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/tests/data"
            "${CMAKE_BINARY_DIR}/tests/bin/benchmarks/data"
    )
    add_dependencies(bench_economics copy_data_bench)
endif()
