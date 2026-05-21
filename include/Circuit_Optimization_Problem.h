/**
 * @file Circuit_Optimization_Problem.h
 * @brief Declares the high-level circuit optimization workflow.
 *
 * This interface packages GA hyperparameters, optimization mode, visualization
 * options, and run metadata behind a single callable entry point used by the
 * command-line executable and tuning tools.
 */

#pragma once

#include <string>
#include <vector>

#include "Genetic_Algorithm.h"

struct Circuit_Optimization_Result {
    double best_fitness = -1e12;
    std::vector<int> best_discrete_solution;
    std::vector<int> best_full_circuit;
    double runtime_seconds = 0.0;
    bool success = false;
};

Circuit_Optimization_Result run_circuit_optimization(const Algorithm_Parameters& params,
                                                     const std::string& mode,
                                                     const std::string& output_image = "",
                                                     bool render_output = false,
                                                     bool verbose = false);
