#include <cstdlib>
#include <iostream>
#include <string>

#include "Circuit_Optimization_Problem.h"

namespace {

bool parse_double(const std::string& text, double& value) {
    char* end = nullptr;
    value = std::strtod(text.c_str(), &end);
    return end != text.c_str() && *end == '\0';
}

}  // namespace

int main(int argc, char* argv[]) {
    Algorithm_Parameters params;
    std::string mode = "fixed";
    std::string output_image = "optimal_circuit.png";

    if (argc > 1) params.population_size = std::stoi(argv[1]);
    if (argc > 2) params.max_iterations = std::stoi(argv[2]);
    if (argc > 3) params.crossover_probability = std::stod(argv[3]);
    if (argc > 4) params.mutation_probability = std::stod(argv[4]);
    if (argc > 5) params.tournament_size = std::stoi(argv[5]);
    if (argc > 6) params.early_stop_patience = std::stoi(argv[6]);
    if (argc > 7) params.num_crossover_points = std::stoi(argv[7]);
    if (argc > 8) params.elite_count = std::stoi(argv[8]);

    if (argc > 9) {
        std::string arg9 = argv[9];
        if (arg9 == "fixed" || arg9 == "swapable") {
            mode = arg9;
            if (argc > 10) output_image = argv[10];
        } else {
            double parsed_gaussian_sigma = 0.0;
            bool arg9_is_gaussian_sigma = parse_double(arg9, parsed_gaussian_sigma);
            if (arg9_is_gaussian_sigma) {
                params.gaussian_sigma = parsed_gaussian_sigma;
            } else {
                output_image = arg9;
            }

            if (argc > 10 && arg9_is_gaussian_sigma) {
                std::string arg10 = argv[10];
                if (arg10 == "fixed" || arg10 == "swapable") {
                    mode = arg10;
                    if (argc > 11) output_image = argv[11];
                } else {
                    output_image = arg10;
                }
            }
        }
    }

    Circuit_Optimization_Result result =
        run_circuit_optimization(params, mode, output_image, true, true);

    return result.success ? 0 : 1;
}
