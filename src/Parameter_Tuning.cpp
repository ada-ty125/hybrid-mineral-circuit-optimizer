/**
 * @file Parameter_Tuning.cpp
 * @brief Runs control-variable sweeps over GA hyperparameters.
 *
 * The executable records repeated optimization runs, writes raw CSV output,
 * and summarizes parameter-level statistics for fixed or swapable circuit
 * optimization modes.
 */

#include <cmath>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

#include "Circuit_Optimization_Problem.h"

namespace {

struct Sweep_Case {
    std::string name;
    std::string value_label;
    Algorithm_Parameters params;
};

struct Run_Record {
    std::string parameter;
    std::string value;
    int repeat = 0;
    double fitness = -1e12;
    double runtime_seconds = 0.0;
    bool success = false;
};

template <typename T>
std::string to_label(T value) {
    std::ostringstream out;
    out << value;
    return out.str();
}

template <>
std::string to_label<double>(double value) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(4) << value;
    return out.str();
}

template <typename T, typename Setter>
void add_sweep_cases(std::vector<Sweep_Case>& cases, const Algorithm_Parameters& base,
                     const std::string& name, const std::vector<T>& values, Setter setter) {
    for (T value : values) {
        Algorithm_Parameters params = base;
        setter(params, value);
        cases.push_back({name, to_label(value), params});
    }
}

std::vector<Sweep_Case> build_control_variable_sweep(const Algorithm_Parameters& base) {
    std::vector<Sweep_Case> cases;

    // add_sweep_cases(cases, base, "max_iterations", std::vector<int>{300, 400, 500, 600},
    //                [](Algorithm_Parameters& p, int v) { p.max_iterations = v; });
    add_sweep_cases(cases, base, "population_size", std::vector<int>{175, 200, 225, 250},
                    [](Algorithm_Parameters& p, int v) { p.population_size = v; });
    // add_sweep_cases(cases, base, "crossover_probability",
    //                std::vector<double>{0.65, 0.70, 0.75, 0.80},
    //                [](Algorithm_Parameters& p, double v) { p.crossover_probability = v; });
    add_sweep_cases(cases, base, "mutation_probability",
                    std::vector<double>{0.10, 0.125, 0.15, 0.175},
                    [](Algorithm_Parameters& p, double v) { p.mutation_probability = v; });
    add_sweep_cases(cases, base, "early_stop_patience", std::vector<int>{50, 75, 100, 125},
                    [](Algorithm_Parameters& p, int v) { p.early_stop_patience = v; });
    add_sweep_cases(cases, base, "tournament_size", std::vector<int>{3, 4, 5, 6},
                    [](Algorithm_Parameters& p, int v) { p.tournament_size = v; });
    add_sweep_cases(cases, base, "num_crossover_points", std::vector<int>{4, 8, 16, 32},
                    [](Algorithm_Parameters& p, int v) { p.num_crossover_points = v; });
    add_sweep_cases(cases, base, "gaussian_sigma", std::vector<double>{0.02, 0.03, 0.04, 0.05},
                    [](Algorithm_Parameters& p, double v) { p.gaussian_sigma = v; });
    // add_sweep_cases(cases, base, "elite_count", std::vector<int>{1, 2, 3, 4},
    //                [](Algorithm_Parameters& p, int v) { p.elite_count = v; });

    return cases;
}

std::string summary_path_for(const std::string& run_path) {
    std::size_t dot = run_path.find_last_of('.');
    if (dot == std::string::npos) return run_path + "_summary.csv";
    return run_path.substr(0, dot) + "_summary" + run_path.substr(dot);
}

void write_raw_header(std::ofstream& out) {
    out << "parameter,value,repeat,fitness,runtime_seconds,success\n";
}

void write_raw_record(std::ofstream& out, const Run_Record& record) {
    out << record.parameter << "," << record.value << "," << record.repeat << ","
        << std::setprecision(12) << record.fitness << "," << record.runtime_seconds << ","
        << (record.success ? 1 : 0) << "\n";
}

void write_summary(const std::string& path, const std::vector<Run_Record>& records) {
    std::ofstream out(path);
    out << "parameter,value,runs,successes,mean_fitness,best_fitness,stddev_fitness,"
           "mean_runtime_seconds\n";

    for (std::size_t i = 0; i < records.size();) {
        std::size_t j = i;
        while (j < records.size() && records[j].parameter == records[i].parameter &&
               records[j].value == records[i].value) {
            ++j;
        }

        int runs = static_cast<int>(j - i);
        int successes = 0;
        double sum_fitness = 0.0;
        double best_fitness = -std::numeric_limits<double>::infinity();
        double sum_runtime = 0.0;

        for (std::size_t k = i; k < j; ++k) {
            successes += records[k].success ? 1 : 0;
            sum_fitness += records[k].fitness;
            best_fitness = std::max(best_fitness, records[k].fitness);
            sum_runtime += records[k].runtime_seconds;
        }

        double mean_fitness = sum_fitness / runs;
        double variance = 0.0;
        for (std::size_t k = i; k < j; ++k) {
            double delta = records[k].fitness - mean_fitness;
            variance += delta * delta;
        }
        double stddev = std::sqrt(variance / runs);

        out << records[i].parameter << "," << records[i].value << "," << runs << "," << successes
            << "," << std::setprecision(12) << mean_fitness << "," << best_fitness << "," << stddev
            << "," << (sum_runtime / runs) << "\n";

        i = j;
    }
}

void print_usage(const char* executable) {
    std::cout << "Usage: " << executable << " [fixed|swapable] [repeats] [raw_output_csv]\n"
              << "Example: " << executable << " fixed 3 parameter_tuning_runs.csv\n";
}

}  // namespace

int main(int argc, char* argv[]) {
    std::string mode = "fixed";
    int repeats = 3;
    std::string raw_output_path = "parameter_tuning_runs.csv";

    if (argc > 1) {
        mode = argv[1];
    }
    if (argc > 2) {
        repeats = std::stoi(argv[2]);
    }
    if (argc > 3) {
        raw_output_path = argv[3];
    }

    if ((mode != "fixed" && mode != "swapable") || repeats < 1) {
        print_usage(argv[0]);
        return 1;
    }

    Algorithm_Parameters base;
    std::vector<Sweep_Case> cases = build_control_variable_sweep(base);
    std::string summary_output_path = summary_path_for(raw_output_path);

    std::ofstream raw_out(raw_output_path);
    if (!raw_out.is_open()) {
        std::cerr << "Could not open output file: " << raw_output_path << "\n";
        return 1;
    }
    write_raw_header(raw_out);

    std::vector<Run_Record> records;
    records.reserve(cases.size() * static_cast<std::size_t>(repeats));

    std::cout << "Control-variable tuning mode: " << mode << "\n";
    std::cout << "Repeats per setting: " << repeats << "\n";
    std::cout << "Raw output: " << raw_output_path << "\n";
    std::cout << "Summary output: " << summary_output_path << "\n";
    std::cout << "Note: gaussian_sigma affects only continuous genes in the current GA path.\n";
    std::cout
        << "Note: elite_count is recorded, but the current GA replacement step does not use it.\n";

    for (const Sweep_Case& sweep_case : cases) {
        std::cout << "\n[" << sweep_case.name << " = " << sweep_case.value_label << "]\n";

        for (int repeat = 1; repeat <= repeats; ++repeat) {
            Circuit_Optimization_Result result =
                run_circuit_optimization(sweep_case.params, mode, "", false, false);

            Run_Record record;
            record.parameter = sweep_case.name;
            record.value = sweep_case.value_label;
            record.repeat = repeat;
            record.fitness = result.best_fitness;
            record.runtime_seconds = result.runtime_seconds;
            record.success = result.success;

            write_raw_record(raw_out, record);
            raw_out.flush();
            records.push_back(record);

            std::cout << "  repeat " << repeat << ": fitness=" << result.best_fitness
                      << ", runtime=" << result.runtime_seconds << "s"
                      << ", success=" << (result.success ? "yes" : "no") << "\n";
        }
    }

    write_summary(summary_output_path, records);
    std::cout << "\nDone. Use the summary CSV to compare mean_fitness and best_fitness.\n";
    return 0;
}
