/**
 * @file main.cpp
 * @brief Command-line entry point for the circuit optimizer executable.
 *
 * Parses genetic-algorithm hyperparameters, circuit search mode, and
 * visualization output options before running the fixed or swapable circuit
 * optimization workflow.
 */

#include <cstdlib>
#include <cerrno>
#include <cctype>
#include <cmath>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "Circuit_Optimization_Problem.h"

namespace {

/**
 * @brief Parsed command-line configuration for one optimizer run.
 */
struct Cli_Config {
    Algorithm_Parameters params;
    std::string mode = "fixed";
    std::string output_image = "optimal_circuit.gif";
    std::string output_format;
    bool output_path_was_set = false;
};

/**
 * @brief Return a lower-case copy of a string.
 *
 * @param value Input text.
 * @return Lower-case copy of `value`.
 */
std::string lower_copy(std::string value) {
    for (char& c : value) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return value;
}

/**
 * @brief Check whether a string names a supported optimization mode.
 *
 * Both `swapable` and the common spelling `swappable` are accepted.
 *
 * @param value Candidate mode string.
 * @return True when `value` is a supported mode.
 */
bool is_mode(const std::string& value) {
    return value == "fixed" || value == "swapable" || value == "swappable";
}

/**
 * @brief Convert accepted mode aliases to the internal spelling.
 *
 * @param value Mode string already accepted by `is_mode`.
 * @return Internal mode name used by `run_circuit_optimization`.
 */
std::string normalize_mode(const std::string& value) {
    return value == "swappable" ? "swapable" : value;
}

/**
 * @brief Normalize an output format token.
 *
 * Converts to lower case and strips one leading dot, so `.PNG` becomes `png`.
 *
 * @param value Format string from CLI or a file extension.
 * @return Normalized output format token.
 */
std::string normalize_output_format(const std::string& value) {
    std::string format = lower_copy(value);
    if (!format.empty() && format.front() == '.') {
        format.erase(format.begin());
    }
    return format;
}

/**
 * @brief Check whether a value names a supported visualization format.
 *
 * @param value Candidate format string.
 * @return True for dot, png, or gif.
 */
bool is_output_format(const std::string& value) {
    const std::string format = normalize_output_format(value);
    return format == "dot" || format == "png" || format == "gif";
}

/**
 * @brief Extract and normalize the extension format from an output path.
 *
 * Directory names containing dots are ignored; only the filename extension is
 * considered.
 *
 * @param path Output path supplied on the command line.
 * @return Normalized extension without a dot, or an empty string when absent.
 */
std::string output_extension_format(const std::string& path) {
    const std::size_t separator_pos = path.find_last_of("/\\");
    const std::size_t dot_pos = path.find_last_of('.');
    if (dot_pos == std::string::npos ||
        (separator_pos != std::string::npos && dot_pos < separator_pos)) {
        return "";
    }
    return normalize_output_format(path.substr(dot_pos + 1));
}

/**
 * @brief Replace an existing output extension or append one if absent.
 *
 * @param path Output path supplied on the command line.
 * @param format Normalized output format without a leading dot.
 * @return Path ending in `format`.
 */
std::string replace_or_append_extension(const std::string& path, const std::string& format) {
    const std::size_t separator_pos = path.find_last_of("/\\");
    const std::size_t dot_pos = path.find_last_of('.');
    if (dot_pos == std::string::npos ||
        (separator_pos != std::string::npos && dot_pos < separator_pos)) {
        return path + "." + format;
    }
    return path.substr(0, dot_pos + 1) + format;
}

/**
 * @brief Parse a base-10 integer with full-string validation.
 *
 * @param text CLI token to parse.
 * @param value Receives the parsed integer on success.
 * @return True when parsing succeeds and fits in `int`.
 */
bool parse_int(const std::string& text, int& value) {
    char* end = nullptr;
    errno = 0;
    long parsed = std::strtol(text.c_str(), &end, 10);
    if (end == text.c_str() || *end != '\0' || errno == ERANGE ||
        parsed < std::numeric_limits<int>::min() || parsed > std::numeric_limits<int>::max()) {
        return false;
    }

    value = static_cast<int>(parsed);
    return true;
}

/**
 * @brief Parse a finite floating-point value with full-string validation.
 *
 * @param text CLI token to parse.
 * @param value Receives the parsed double on success.
 * @return True when parsing succeeds and the result is finite.
 */
bool parse_double(const std::string& text, double& value) {
    char* end = nullptr;
    errno = 0;
    value = std::strtod(text.c_str(), &end);
    return end != text.c_str() && *end == '\0' && errno != ERANGE && std::isfinite(value);
}

/**
 * @brief Parse and assign a bounded integer option.
 *
 * @param text CLI value token.
 * @param name Human-readable option name for error messages.
 * @param min_value Inclusive lower bound.
 * @param target Destination parameter.
 * @param error Receives a diagnostic on failure.
 * @return True on successful assignment.
 */
bool assign_int(const std::string& text, const std::string& name, int min_value, int& target,
                std::string& error) {
    int parsed = 0;
    if (!parse_int(text, parsed) || parsed < min_value) {
        error = name + " must be an integer >= " + std::to_string(min_value) + ": " + text;
        return false;
    }

    target = parsed;
    return true;
}

/**
 * @brief Parse and assign a bounded floating-point option.
 *
 * @param text CLI value token.
 * @param name Human-readable option name for error messages.
 * @param min_value Inclusive lower bound.
 * @param max_value Inclusive upper bound.
 * @param target Destination parameter.
 * @param error Receives a diagnostic on failure.
 * @return True on successful assignment.
 */
bool assign_double(const std::string& text, const std::string& name, double min_value,
                   double max_value, double& target, std::string& error) {
    double parsed = 0.0;
    if (!parse_double(text, parsed) || parsed < min_value || parsed > max_value) {
        error = name + " must be a finite number in [" + std::to_string(min_value) + ", " +
                std::to_string(max_value) + "]: " + text;
        return false;
    }

    target = parsed;
    return true;
}

/**
 * @brief Validate and assign the circuit search mode.
 *
 * @param text Candidate mode string.
 * @param mode Destination mode string.
 * @param error Receives a diagnostic on failure.
 * @return True on successful assignment.
 */
bool assign_mode(const std::string& text, std::string& mode, std::string& error) {
    if (!is_mode(text)) {
        error = "mode must be fixed or swapable: " + text;
        return false;
    }

    mode = normalize_mode(text);
    return true;
}

/**
 * @brief Record an output path supplied by the user.
 *
 * @param text Output path.
 * @param config CLI configuration to update.
 */
void assign_output_path(const std::string& text, Cli_Config& config) {
    config.output_image = text;
    config.output_path_was_set = true;
}

/**
 * @brief Validate and assign the requested output format.
 *
 * @param text Format token from `--output-format` or `--format`.
 * @param config CLI configuration to update.
 * @param error Receives a diagnostic on failure.
 * @return True on successful assignment.
 */
bool assign_output_format(const std::string& text, Cli_Config& config, std::string& error) {
    if (!is_output_format(text)) {
        error = "output format must be dot, png, or gif: " + text;
        return false;
    }

    config.output_format = normalize_output_format(text);
    return true;
}

/**
 * @brief Check whether a long option consumes a value token.
 *
 * @param option Long option name without leading dashes.
 * @return True when the option requires a value.
 */
bool option_requires_value(const std::string& option) { return option != "help"; }

/**
 * @brief Apply one parsed named option to the CLI configuration.
 *
 * @param option Long option name without leading dashes.
 * @param value Option value token.
 * @param config CLI configuration to update.
 * @param error Receives a diagnostic on failure.
 * @return True when the option is known and valid.
 */
bool apply_option(const std::string& option, const std::string& value, Cli_Config& config,
                  std::string& error) {
    if (option == "population-size" || option == "population" || option == "pop") {
        return assign_int(value, "--" + option, 1, config.params.population_size, error);
    }
    if (option == "max-iterations" || option == "iterations" || option == "iter") {
        return assign_int(value, "--" + option, 1, config.params.max_iterations, error);
    }
    if (option == "crossover-probability" || option == "crossover-prob" || option == "cross") {
        return assign_double(value, "--" + option, 0.0, 1.0, config.params.crossover_probability,
                             error);
    }
    if (option == "mutation-probability" || option == "mutation-prob" || option == "mut") {
        return assign_double(value, "--" + option, 0.0, 1.0, config.params.mutation_probability,
                             error);
    }
    if (option == "tournament-size" || option == "tournament") {
        return assign_int(value, "--" + option, 1, config.params.tournament_size, error);
    }
    if (option == "early-stop-patience" || option == "patience") {
        return assign_int(value, "--" + option, 1, config.params.early_stop_patience, error);
    }
    if (option == "num-crossover-points" || option == "crossover-points" || option == "points") {
        return assign_int(value, "--" + option, 0, config.params.num_crossover_points, error);
    }
    if (option == "elite-count" || option == "elite") {
        return assign_int(value, "--" + option, 0, config.params.elite_count, error);
    }
    if (option == "gaussian-sigma" || option == "sigma") {
        return assign_double(value, "--" + option, 0.0, std::numeric_limits<double>::max(),
                             config.params.gaussian_sigma, error);
    }
    if (option == "mode") {
        return assign_mode(value, config.mode, error);
    }
    if (option == "output" || option == "output-image") {
        if (value.empty()) {
            error = "--" + option + " must not be empty";
            return false;
        }
        assign_output_path(value, config);
        return true;
    }
    if (option == "output-format" || option == "format") {
        return assign_output_format(value, config, error);
    }

    error = "unknown option: --" + option;
    return false;
}

/**
 * @brief Parse the legacy positional argument layout.
 *
 * This preserves compatibility with tuning scripts that pass GA parameters by
 * position instead of using named options.
 *
 * @param args Positional tokens after named options have been removed.
 * @param config CLI configuration to update.
 * @param error Receives a diagnostic on failure.
 * @return True when all positional tokens are valid.
 */
bool parse_legacy_positionals(const std::vector<std::string>& args, Cli_Config& config,
                              std::string& error) {
    if (args.empty()) {
        return true;
    }

    if (is_mode(args[0])) {
        if (!assign_mode(args[0], config.mode, error)) return false;
        if (args.size() > 1) assign_output_path(args[1], config);
        if (args.size() > 2) {
            error = "too many positional arguments after mode; use named options for GA params";
            return false;
        }
        return true;
    }

    if (args.size() > 11) {
        error = "too many positional arguments";
        return false;
    }

    if (args.size() > 0 &&
        !assign_int(args[0], "population_size", 1, config.params.population_size, error)) {
        return false;
    }
    if (args.size() > 1 &&
        !assign_int(args[1], "max_iterations", 1, config.params.max_iterations, error)) {
        return false;
    }
    if (args.size() > 2 && !assign_double(args[2], "crossover_probability", 0.0, 1.0,
                                          config.params.crossover_probability, error)) {
        return false;
    }
    if (args.size() > 3 && !assign_double(args[3], "mutation_probability", 0.0, 1.0,
                                          config.params.mutation_probability, error)) {
        return false;
    }
    if (args.size() > 4 &&
        !assign_int(args[4], "tournament_size", 1, config.params.tournament_size, error)) {
        return false;
    }
    if (args.size() > 5 &&
        !assign_int(args[5], "early_stop_patience", 1, config.params.early_stop_patience, error)) {
        return false;
    }
    if (args.size() > 6 && !assign_int(args[6], "num_crossover_points", 0,
                                       config.params.num_crossover_points, error)) {
        return false;
    }
    if (args.size() > 7 &&
        !assign_int(args[7], "elite_count", 0, config.params.elite_count, error)) {
        return false;
    }

    if (args.size() > 8) {
        if (is_mode(args[8])) {
            if (!assign_mode(args[8], config.mode, error)) return false;
            if (args.size() > 9) assign_output_path(args[9], config);
            if (args.size() > 10) {
                error = "too many positional arguments after output path";
                return false;
            }
        } else {
            double parsed_sigma = 0.0;
            if (parse_double(args[8], parsed_sigma)) {
                if (!assign_double(args[8], "gaussian_sigma", 0.0,
                                   std::numeric_limits<double>::max(), config.params.gaussian_sigma,
                                   error)) {
                    return false;
                }
                if (args.size() > 9) {
                    if (is_mode(args[9])) {
                        if (!assign_mode(args[9], config.mode, error)) return false;
                        if (args.size() > 10) assign_output_path(args[10], config);
                    } else {
                        assign_output_path(args[9], config);
                    }
                }
            } else {
                assign_output_path(args[8], config);
                if (args.size() > 9) {
                    error = "too many positional arguments after output path";
                    return false;
                }
            }
        }
    }

    return true;
}

/**
 * @brief Parse command-line arguments into a run configuration.
 *
 * Supports both named `--option value` / `--option=value` syntax and the
 * original positional parameter order.
 *
 * @param argc Argument count from `main`.
 * @param argv Argument vector from `main`.
 * @param config CLI configuration to update.
 * @param show_help Set to true when help was requested.
 * @param error Receives a diagnostic on failure.
 * @return True when parsing succeeds or help was requested.
 */
bool parse_args(int argc, char* argv[], Cli_Config& config, bool& show_help, std::string& error) {
    std::vector<std::string> positional_args;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            show_help = true;
            return true;
        }

        if (arg.rfind("--", 0) == 0) {
            std::string option = arg.substr(2);
            std::string value;

            const std::size_t equals_pos = option.find('=');
            if (equals_pos != std::string::npos) {
                value = option.substr(equals_pos + 1);
                option = option.substr(0, equals_pos);
            } else if (option_requires_value(option)) {
                if (i + 1 >= argc) {
                    error = "missing value for --" + option;
                    return false;
                }
                value = argv[++i];
            }

            if (!apply_option(option, value, config, error)) {
                return false;
            }
        } else {
            positional_args.push_back(arg);
        }
    }

    return parse_legacy_positionals(positional_args, config, error);
}

/**
 * @brief Reconcile output path and explicit output format options.
 *
 * Adds an extension when `--output-format` is used with an extensionless path,
 * validates supported extensions, and rejects conflicting path/format pairs.
 *
 * @param config CLI configuration to update.
 * @param error Receives a diagnostic on failure.
 * @return True when output settings are internally consistent.
 */
bool finalize_output_options(Cli_Config& config, std::string& error) {
    const std::string extension = output_extension_format(config.output_image);

    if (!config.output_format.empty()) {
        if (!config.output_path_was_set || extension.empty()) {
            config.output_image =
                replace_or_append_extension(config.output_image, config.output_format);
            return true;
        }

        if (extension != config.output_format) {
            error = "output extension ." + extension + " conflicts with --output-format " +
                    config.output_format;
            return false;
        }

        return true;
    }

    if (extension.empty() || is_output_format(extension)) {
        return true;
    }

    error = "unsupported output extension ." + extension + "; use .dot, .png, or .gif";
    return false;
}

/**
 * @brief Print command-line usage and examples.
 *
 * @param executable Program name from `argv[0]`.
 */
void print_usage(const char* executable) {
    std::cout
        << "Usage:\n"
        << "  " << executable << " [options]\n"
        << "  " << executable
        << " population iterations crossover mutation tournament patience points elite "
           "[sigma] [fixed|swapable] [output]\n\n"
        << "Options:\n"
        << "  --population-size N      Population size, >= 1\n"
        << "  --max-iterations N       Maximum GA generations, >= 1\n"
        << "  --crossover-probability P  Crossover probability in [0, 1]\n"
        << "  --mutation-probability P   Mutation probability in [0, 1]\n"
        << "  --tournament-size N      Tournament size, >= 1\n"
        << "  --early-stop-patience N  Early-stop patience, >= 1\n"
        << "  --num-crossover-points N Crossover points, >= 0\n"
        << "  --elite-count N          Elite count, >= 0\n"
        << "  --gaussian-sigma X       Gaussian mutation sigma, >= 0\n"
        << "  --mode fixed|swapable    Circuit search mode\n"
        << "  --output PATH            Output image path (.gif, .png, or .dot)\n"
        << "  --output-format FMT      Output format: dot, png, or gif\n"
        << "  --help                   Show this message\n\n"
        << "If --output has no extension and --output-format is omitted, DOT text is written.\n\n"
        << "Examples:\n"
        << "  " << executable
        << " --population-size 16 --max-iterations 3 --mode fixed --output smoke.png\n"
        << "  " << executable
        << " --population-size 16 --max-iterations 3 --output smoke --format gif\n"
        << "  " << executable << " 16 3 0.935 0.214 2 3 1 1 fixed smoke.png\n";
}

}  // namespace

/**
 * @brief Program entry point for the circuit optimizer executable.
 *
 * @param argc Command-line argument count.
 * @param argv Command-line argument values.
 * @return Zero on successful optimization, non-zero on invalid CLI input or
 * optimization failure.
 */
int main(int argc, char* argv[]) {
    Cli_Config config;
    bool show_help = false;
    std::string error;

    if (!parse_args(argc, argv, config, show_help, error)) {
        std::cerr << "Error: " << error << "\n\n";
        print_usage(argv[0]);
        return 1;
    }

    if (show_help) {
        print_usage(argv[0]);
        return 0;
    }

    if (!finalize_output_options(config, error)) {
        std::cerr << "Error: " << error << "\n\n";
        print_usage(argv[0]);
        return 1;
    }

    Circuit_Optimization_Result result =
        run_circuit_optimization(config.params, config.mode, config.output_image, true, true);

    return result.success ? 0 : 1;
}
