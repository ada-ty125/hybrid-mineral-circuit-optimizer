#!/usr/bin/env bash
set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

MODE="fixed"
REPEATS=5
OUT_DIR="$REPO_ROOT/build/ga_benchmark"
CHECK_ONLY=0
BASE_POP=100
BASE_ITER=200
BASE_CROSS=0.9
BASE_MUT=0.01
BASE_TOURN=3
BASE_PAT=50
BASE_PTS=2
BASE_ELITE=1

ALGORITHMS=()
CPP_SOURCES=()
EXECUTABLES=()

usage() {
    cat <<EOF
Usage:
  $0 --cpp Name:path.cpp [--cpp Name:path.cpp ...] [options]
  $0 --exe Name:path/to/Circuit_Optimizer [--exe Name:path ...] [options]

Options:
  --repeats N          Runs per experiment setting. Default: 5
  --mode fixed|swapable
  --out-dir DIR        Default: build/ga_benchmark
  --check-only         Only compile and smoke test algorithms, then exit
  --help

Examples:
  $0 --cpp Baseline:benchmarks/variants/Genetic_Algorithmbaseline.cpp \\
     --cpp OpenMP:benchmarks/variants/Genetic_AlgorithmNaiveOPenMP.cpp \\
     --cpp Algo3:benchmarks/variants/Genetic_AlgorithmAlgo3.cpp \\
     --cpp Algo4:src/Genetic_Algorithm.cpp --repeats 5

  $0 --exe Current:build/bin/Circuit_Optimizer --repeats 10

Each executable must accept the same arguments as src/main.cpp:
  pop iter crossover mutation tournament patience crossover_points elite mode output_image
EOF
}

die() {
    echo "error: $*" >&2
    exit 1
}

sanitize_name() {
    echo "$1" | tr -c '[:alnum:]_.-' '_' | sed 's/^_*//; s/_*$//'
}

abs_path() {
    local path="$1"
    if [[ "$path" = /* ]]; then
        echo "$path"
    else
        echo "$REPO_ROOT/$path"
    fi
}

add_algorithm() {
    local kind="$1"
    local spec="$2"
    [[ "$spec" == *:* ]] || die "$kind spec must be Name:path, got '$spec'"
    local name="${spec%%:*}"
    local path="${spec#*:}"
    [[ -n "$name" && -n "$path" ]] || die "$kind spec must be Name:path, got '$spec'"
    ALGORITHMS+=("$name")
    if [[ "$kind" == "cpp" ]]; then
        CPP_SOURCES+=("$(abs_path "$path")")
        EXECUTABLES+=("")
    else
        CPP_SOURCES+=("")
        EXECUTABLES+=("$(abs_path "$path")")
    fi
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --cpp)
            [[ $# -ge 2 ]] || die "--cpp needs Name:path.cpp"
            add_algorithm cpp "$2"
            shift 2
            ;;
        --exe|--executable)
            [[ $# -ge 2 ]] || die "--exe needs Name:path"
            add_algorithm exe "$2"
            shift 2
            ;;
        --repeats)
            [[ $# -ge 2 ]] || die "--repeats needs a number"
            REPEATS="$2"
            shift 2
            ;;
        --mode)
            [[ $# -ge 2 ]] || die "--mode needs fixed or swapable"
            MODE="$2"
            shift 2
            ;;
        --out-dir)
            [[ $# -ge 2 ]] || die "--out-dir needs a directory"
            OUT_DIR="$(abs_path "$2")"
            shift 2
            ;;
        --check-only)
            CHECK_ONLY=1
            shift
            ;;
        --help|-h)
            usage
            exit 0
            ;;
        *)
            die "unknown argument: $1"
            ;;
    esac
done

[[ "$MODE" == "fixed" || "$MODE" == "swapable" ]] || die "--mode must be fixed or swapable"
[[ "$REPEATS" =~ ^[0-9]+$ && "$REPEATS" -ge 1 ]] || die "--repeats must be a positive integer"
[[ "${#ALGORITHMS[@]}" -gt 0 ]] || die "provide at least one --cpp or --exe algorithm"

mkdir -p "$OUT_DIR"
RAW_CSV="$OUT_DIR/ga_benchmark_runs.csv"
SUMMARY_CSV="$OUT_DIR/ga_benchmark_summary.csv"
LOG_DIR="$OUT_DIR/logs"
PLOT_DIR="$OUT_DIR/plots"
mkdir -p "$LOG_DIR" "$PLOT_DIR"

echo "Algorithm,Experiment,Parameter,Value,Repeat,Mode,Population,Iterations,CrossoverProb,MutationProb,Tournament,Patience,CrossPoints,Elite,Score,RuntimeSeconds,Success" > "$RAW_CSV"

build_cpp_algorithm() {
    local name="$1"
    local source="$2"
    local label
    label="$(sanitize_name "$name")"
    [[ -f "$source" ]] || die "cannot find cpp source for $name: $source"

    local build_dir="$OUT_DIR/builds/$label"
    mkdir -p "$build_dir"
    echo "Compiling $name from $source" >&2
    cmake -S "$REPO_ROOT" -B "$build_dir" -DGA_SOURCE_FILE="$source" >/dev/null || return 1
    cmake --build "$build_dir" --target Circuit_Optimizer -j >/dev/null || return 1
    echo "$build_dir/bin/Circuit_Optimizer"
}

smoke_test_algorithm() {
    local name="$1"
    local exec_path="$2"
    local log="$LOG_DIR/$(sanitize_name "$name")_smoke_test.log"
    local image="$PLOT_DIR/$(sanitize_name "$name")_smoke_test.dot"

    echo "Smoke testing $name" >&2
    # Keep preflight bounded. The legacy baseline can spend minutes in a 200-generation
    # optimization, while three generations are sufficient to validate the executable contract.
    "$exec_path" 16 3 "$BASE_CROSS" "$BASE_MUT" "$BASE_TOURN" 3 "$BASE_PTS" \
        "$BASE_ELITE" "$MODE" "$image" > "$log" 2>&1
    local exit_code=$?

    local score
    local runtime
    score="$(awk '/Best performance score/ {print $NF}' "$log" | tail -n 1)"
    runtime="$(awk '/Optimization runtime:/ {print $3}' "$log" | tail -n 1)"

    if [[ "$exit_code" -ne 0 || -z "$score" || -z "$runtime" ]]; then
        echo "Smoke test failed for $name; see $log" >&2
        return 1
    fi
}

csv_append() {
    local algorithm="$1"
    local experiment="$2"
    local parameter="$3"
    local value="$4"
    local repeat="$5"
    local pop="$6"
    local iter="$7"
    local cross="$8"
    local mut="$9"
    local score="${10}"
    local runtime="${11}"
    local success="${12}"

    printf '%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n' \
        "$algorithm" "$experiment" "$parameter" "$value" "$repeat" "$MODE" \
        "$pop" "$iter" "$cross" "$mut" "$BASE_TOURN" "$BASE_PAT" "$BASE_PTS" \
        "$BASE_ELITE" "$score" "$runtime" "$success" >> "$RAW_CSV"
}

run_experiment() {
    local algorithm="$1"
    local exec_path="$2"
    local experiment="$3"
    local parameter="$4"
    local value="$5"
    local pop="$6"
    local iter="$7"
    local cross="$8"
    local mut="$9"

    for repeat in $(seq 1 "$REPEATS"); do
        local image="$PLOT_DIR/$(sanitize_name "$algorithm")_$(sanitize_name "$experiment")_r${repeat}.png"
        local log="$LOG_DIR/$(sanitize_name "$algorithm")_$(sanitize_name "$experiment")_r${repeat}.log"

        echo "[$algorithm] $experiment repeat $repeat/$REPEATS"
        "$exec_path" "$pop" "$iter" "$cross" "$mut" "$BASE_TOURN" "$BASE_PAT" \
            "$BASE_PTS" "$BASE_ELITE" "$MODE" "$image" > "$log" 2>&1
        local exit_code=$?

        local score
        local runtime
        score="$(awk '/Best performance score/ {print $NF}' "$log" | tail -n 1)"
        runtime="$(awk '/Optimization runtime:/ {print $3}' "$log" | tail -n 1)"
        [[ -n "$score" ]] || score="NA"
        [[ -n "$runtime" ]] || runtime="NA"

        if [[ "$exit_code" -eq 0 && "$score" != "NA" && "$runtime" != "NA" ]]; then
            csv_append "$algorithm" "$experiment" "$parameter" "$value" "$repeat" \
                "$pop" "$iter" "$cross" "$mut" "$score" "$runtime" "1"
        else
            echo "warning: $algorithm $experiment repeat $repeat failed; see $log" >&2
            csv_append "$algorithm" "$experiment" "$parameter" "$value" "$repeat" \
                "$pop" "$iter" "$cross" "$mut" "$score" "$runtime" "0"
        fi
    done
}

write_summary_and_tables() {
    awk -F, '
    NR == 1 { next }
    $15 == "NA" || $16 == "NA" { next }
    {
        key = $1 SUBSEP $2 SUBSEP $3 SUBSEP $4
        alg[key] = $1
        exp_name[key] = $2
        param[key] = $3
        val[key] = $4
        score = $15 + 0
        runtime = $16 + 0
        n[key]++
        success[key] += ($17 + 0)
        sum_score[key] += score
        sumsq_score[key] += score * score
        sum_runtime[key] += runtime
        sumsq_runtime[key] += runtime * runtime
        if (!(key in min_score) || score < min_score[key]) min_score[key] = score
        if (!(key in max_score) || score > max_score[key]) max_score[key] = score
        if (!(key in min_runtime) || runtime < min_runtime[key]) min_runtime[key] = runtime
        if (!(key in max_runtime) || runtime > max_runtime[key]) max_runtime[key] = runtime
    }
    END {
        print "Algorithm,Experiment,Parameter,Value,Runs,Successes,MeanScore,StdDevScore,MinScore,MaxScore,MeanRuntimeSeconds,StdDevRuntimeSeconds,MinRuntimeSeconds,MaxRuntimeSeconds"
        for (key in n) {
            mean_s = sum_score[key] / n[key]
            var_s = (sumsq_score[key] / n[key]) - (mean_s * mean_s)
            if (var_s < 0) var_s = 0
            mean_t = sum_runtime[key] / n[key]
            var_t = (sumsq_runtime[key] / n[key]) - (mean_t * mean_t)
            if (var_t < 0) var_t = 0
            printf "%s,%s,%s,%s,%d,%d,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g\n",
                alg[key], exp_name[key], param[key], val[key], n[key], success[key],
                mean_s, sqrt(var_s), min_score[key], max_score[key],
                mean_t, sqrt(var_t), min_runtime[key], max_runtime[key]
        }
    }' "$RAW_CSV" > "$SUMMARY_CSV"

    awk -F, '
    NR == 1 { next }
    {
        score = $7 + 0
        runtime = $11 + 0
        n[$1]++
        sum_score[$1] += score
        sumsq_score[$1] += score * score
        sum_runtime[$1] += runtime
        if (!($1 in hi_score) || score > hi_score[$1]) { hi_score[$1] = score; hi_exp[$1] = $2 }
        if (!($1 in lo_score) || score < lo_score[$1]) { lo_score[$1] = score; lo_exp[$1] = $2 }
        if (!($1 in fast) || runtime < fast[$1]) { fast[$1] = runtime; fast_exp[$1] = $2 }
        if (!($1 in slow) || runtime > slow[$1]) { slow[$1] = runtime; slow_exp[$1] = $2 }
    }
    END {
        for (a in n) {
            mean_s = sum_score[a] / n[a]
            var_s = (sumsq_score[a] / n[a]) - (mean_s * mean_s)
            if (var_s < 0) var_s = 0
            print ""
            print "Algorithm: " a
            print "Score Statistics:"
            printf "  Highest Score: %.3f (Experiment: %s)\n", hi_score[a], hi_exp[a]
            printf "  Lowest Score:  %.3f (Experiment: %s)\n", lo_score[a], lo_exp[a]
            printf "  Mean Score:    %.3f\n", mean_s
            printf "  Std Dev:       %.3f\n", sqrt(var_s)
            print ""
            print "Runtime Statistics:"
            printf "  Fastest:  %.3fs (Experiment: %s)\n", fast[a], fast_exp[a]
            printf "  Slowest:  %.3fs (Experiment: %s)\n", slow[a], slow_exp[a]
            printf "  Average:  %.3fs\n", sum_runtime[a] / n[a]
        }
    }' "$SUMMARY_CSV"
}

echo "Starting GA variant benchmark..."
echo "Mode: $MODE"
echo "Repeats per experiment: $REPEATS"
echo "Raw CSV: $RAW_CSV"

echo "Prebuilding and smoke testing all algorithms..."
for index in "${!ALGORITHMS[@]}"; do
    name="${ALGORITHMS[$index]}"
    exec_path="${EXECUTABLES[$index]}"
    source_path="${CPP_SOURCES[$index]}"

    if [[ -n "$source_path" ]]; then
        exec_path="$(build_cpp_algorithm "$name" "$source_path")" || die "failed to build $name"
        EXECUTABLES[$index]="$exec_path"
    else
        [[ -x "$exec_path" ]] || die "executable for $name is not runnable: $exec_path"
    fi
    smoke_test_algorithm "$name" "$exec_path" || die "$name failed smoke test"
done

if [[ "$CHECK_ONLY" -eq 1 ]]; then
    echo "All algorithms built and passed smoke tests."
    echo "Check-only mode complete. No benchmark runs were started."
    exit 0
fi

echo "All algorithms built and passed smoke tests. Starting benchmark runs..."
for index in "${!ALGORITHMS[@]}"; do
    name="${ALGORITHMS[$index]}"
    exec_path="${EXECUTABLES[$index]}"

    for POP in 50 100 200 500 1000 2000; do
        run_experiment "$name" "$exec_path" "Pop_Size_$POP" "population_size" "$POP" \
            "$POP" "$BASE_ITER" "$BASE_CROSS" "$BASE_MUT"
    done

    for ITER in 50 100 200 500 1000 2000; do
        run_experiment "$name" "$exec_path" "Max_Iter_$ITER" "max_iterations" "$ITER" \
            "$BASE_POP" "$ITER" "$BASE_CROSS" "$BASE_MUT"
    done

    for CROSS in 0.8 0.85 0.9 0.95 1.0; do
        run_experiment "$name" "$exec_path" "Cross_Prob_$CROSS" "crossover_probability" "$CROSS" \
            "$BASE_POP" "$BASE_ITER" "$CROSS" "$BASE_MUT"
    done

    for MUT in 0.001 0.005 0.01 0.05 0.1; do
        run_experiment "$name" "$exec_path" "Mut_Prob_$MUT" "mutation_probability" "$MUT" \
            "$BASE_POP" "$BASE_ITER" "$BASE_CROSS" "$MUT"
    done
done

write_summary_and_tables

echo ""
echo "Benchmark complete."
echo "Raw CSV:     $RAW_CSV"
echo "Summary CSV: $SUMMARY_CSV"
echo "Plot with:   python3 scripts/plot_ga_benchmark.py $SUMMARY_CSV -o $OUT_DIR/ga_benchmark_scores.png"
