list (APPEND genetic_algorithm_tests test_genetic_algorithm
                                     test_prng
     )

add_tests("${genetic_algorithm_tests}" "genetic_algorithm" geneticAlgorithm esegraph)