list(APPEND circuit_simulator_tests test_circuit_simulator
                                     test_validity_checker
    )

add_tests("${circuit_simulator_tests}" "simulator" circuitSimulator esegraph)