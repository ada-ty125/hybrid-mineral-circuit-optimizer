list(APPEND circuit_simulator_tests test_circuit_simulator
                                     test_validity_checker
                                     test_tailings_product_routing
    )

add_tests("${circuit_simulator_tests}" "simulator" circuitSimulator esegraph)

# Economics module tests. Linked directly to circuitEconomics rather than the
# full circuitSimulator so they remain buildable while the rest of the
# simulator is WIP and esegraph is unavailable.
add_executable(test_economics test_economics.cpp)
target_link_libraries(test_economics PRIVATE circuitEconomics)
set_target_properties(test_economics PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/bin")
add_test(NAME test_economics COMMAND test_economics)
set_property(TEST test_economics PROPERTY LABELS simulator)

add_executable(test_new_simulator test_new_simulator.cpp)
target_link_libraries(test_new_simulator circuitSimulator esegraph)
target_include_directories(test_new_simulator PRIVATE ${PROJECT_SOURCE_DIR}/includes)
add_test(NAME test_new_simulator COMMAND test_new_simulator)

add_executable(test_cunit test_cunit.cpp)
target_link_libraries(test_cunit circuitSimulator esegraph)
target_include_directories(test_cunit PRIVATE ${PROJECT_SOURCE_DIR}/includes)
add_test(NAME test_cunit COMMAND test_cunit)