#!/usr/bin/env bash
set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

RUNS=40
BASE_SEED=20260521
OUT_DIR="$REPO_ROOT/build/ga_reliability_algo4"
MODE="fixed"
FAST=0
RUN_TIMEOUT_SECONDS=30

FAST_RUNS=10
FAST_POP=100
FAST_ITER=120
FAST_PATIENCE=40

POP=362
ITER=988
CROSS=0.9350
MUT=0.2142
TOURN=2
PATIENCE=330
CROSS_POINTS=1
ELITE=5
SIGMA=0.2709

usage() {
    cat <<EOF
Usage:
    $0 [--runs N] [--base-seed N] [--out-dir DIR] [--fast]

Runs Algorithm 4 reliability tests with fixed parameters:
  population_size=$POP
  max_iterations=$ITER
  crossover_probability=$CROSS
  mutation_probability=$MUT
  tournament_size=$TOURN
  early_stop_patience=$PATIENCE
  num_crossover_points=$CROSS_POINTS
  elite_count=$ELITE
  gaussian_sigma=$SIGMA

Fast mode:
    --fast uses a smaller run count and disables progress logging so you can
    get a single approximate reliability chart quickly.

Per-run cap:
    Each optimizer invocation is stopped after $RUN_TIMEOUT_SECONDS seconds.
EOF
}

die() {
    echo "error: $*" >&2
    exit 1
}

run_with_timeout() {
    local timeout_seconds="$1"
    local log_path="$2"
    shift 2

    python3 - "$timeout_seconds" "$log_path" "$@" <<'PY'
import subprocess
import sys

timeout_seconds = float(sys.argv[1])
log_path = sys.argv[2]
cmd = sys.argv[3:]

with open(log_path, "w") as log_handle:
    try:
        completed = subprocess.run(
            cmd,
            stdout=log_handle,
            stderr=subprocess.STDOUT,
            timeout=timeout_seconds,
            check=False,
        )
        raise SystemExit(completed.returncode)
    except subprocess.TimeoutExpired:
        log_handle.write(f"\n[runner] timeout after {timeout_seconds} seconds\n")
        log_handle.flush()
        raise SystemExit(124)
PY
}

abs_path() {
    local path="$1"
    if [[ "$path" = /* ]]; then
        echo "$path"
    else
        echo "$REPO_ROOT/$path"
    fi
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --runs)
            [[ $# -ge 2 ]] || die "--runs needs a number"
            RUNS="$2"
            shift 2
            ;;
        --base-seed)
            [[ $# -ge 2 ]] || die "--base-seed needs a number"
            BASE_SEED="$2"
            shift 2
            ;;
        --out-dir)
            [[ $# -ge 2 ]] || die "--out-dir needs a directory"
            OUT_DIR="$(abs_path "$2")"
            shift 2
            ;;
        --fast)
            FAST=1
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

[[ "$RUNS" =~ ^[0-9]+$ && "$RUNS" -ge 1 ]] || die "--runs must be a positive integer"
[[ "$BASE_SEED" =~ ^[0-9]+$ ]] || die "--base-seed must be a non-negative integer"

if [[ "$FAST" -eq 1 ]]; then
    if [[ "$RUNS" -eq 40 ]]; then
        RUNS="$FAST_RUNS"
    fi
    POP="$FAST_POP"
    ITER="$FAST_ITER"
    PATIENCE="$FAST_PATIENCE"
fi

PLOT_TITLE="GA reliability - N=${POP}, crossover=${CROSS}, mutation=${MUT} (different seeds)"
PLOT_PARAMETERS=$(cat <<EOF
population_size=${POP}   max_iterations=${ITER}   crossover_probability=${CROSS}
mutation_probability=${MUT}   tournament_size=${TOURN}   early_stop_patience=${PATIENCE}
num_crossover_points=${CROSS_POINTS}   elite_count=${ELITE}   gaussian_sigma=${SIGMA}
EOF
)

BUILD_DIR="$OUT_DIR/build_algo4"
LOG_DIR="$OUT_DIR/logs"
CONVERGENCE_DIR="$OUT_DIR/convergence"
PLOT_DIR="$OUT_DIR/plots"
RUN_CSV="$OUT_DIR/algo4_reliability_runs.csv"
PLOT_OUT="$OUT_DIR/algo4_reliability.png"

mkdir -p "$BUILD_DIR" "$LOG_DIR" "$CONVERGENCE_DIR" "$PLOT_DIR"

echo "Compiling Algorithm 4 from src/Genetic_Algorithm.cpp"
cmake -S "$REPO_ROOT" -B "$BUILD_DIR" -DGA_SOURCE_FILE="$REPO_ROOT/src/Genetic_Algorithm.cpp" >/dev/null || die "cmake configure failed"
cmake --build "$BUILD_DIR" --target Circuit_Optimizer -j >/dev/null || die "cmake build failed"

EXEC="$BUILD_DIR/bin/Circuit_Optimizer"
[[ -x "$EXEC" ]] || die "compiled executable is not runnable: $EXEC"

echo "Run,Seed,Score,RuntimeSeconds,FinalGeneration,BestGeneration,BestScore,WorstGeneration,WorstScore,ConvergenceCsv,LogFile" > "$RUN_CSV"

echo "Starting Algorithm 4 reliability study..."
echo "Runs: $RUNS"
echo "Output: $OUT_DIR"

for run in $(seq 1 "$RUNS"); do
    seed=$((BASE_SEED + run * 1009))
    run_label="$(printf "%03d" "$run")"
    convergence_csv="$CONVERGENCE_DIR/run_${run_label}_seed_${seed}.csv"
    log_file="$LOG_DIR/run_${run_label}_seed_${seed}.log"

    echo "[Algo4 reliability] run $run/$RUNS seed=$seed"
    start_seconds="$(date +%s)"
    progress_every=25
    if [[ "$FAST" -eq 1 ]]; then
        progress_every=0
    fi
    GA_SEED="$seed" GA_CONVERGENCE_CSV="$convergence_csv" GA_PROGRESS_EVERY="$progress_every" \
        run_with_timeout "$RUN_TIMEOUT_SECONDS" "$log_file" \
        "$EXEC" "$POP" "$ITER" "$CROSS" "$MUT" "$TOURN" "$PATIENCE" \
        "$CROSS_POINTS" "$ELITE" "$SIGMA" "$MODE" ""
    exit_code=$?
    end_seconds="$(date +%s)"

    score="$(awk '/Best performance score/ {print $NF}' "$log_file" | tail -n 1)"
    runtime="$(awk '/Optimization runtime:/ {print $3}' "$log_file" | tail -n 1)"
    echo "  finished in $((end_seconds - start_seconds))s; score=${score:-NA}; optimizer_runtime=${runtime:-NA}s"

    if [[ "$exit_code" -ne 0 || -z "$score" || -z "$runtime" || ! -s "$convergence_csv" ]]; then
        echo "warning: run $run failed or has incomplete output; see $log_file" >&2
        score="${score:-NA}"
        runtime="${runtime:-NA}"
        echo "$run,$seed,$score,$runtime,NA,NA,NA,NA,NA,$convergence_csv,$log_file" >> "$RUN_CSV"
        continue
    fi

    final_generation="$(awk -F, 'NR > 1 {gen=$1} END {print gen}' "$convergence_csv")"
    best_pair="$(awk -F, 'NR > 1 && ($3 + 0) > best {best=$3 + 0; gen=$1} END {printf "%s %.12g", gen, best}' "$convergence_csv")"
    worst_pair="$(awk -F, 'NR > 1 && (seen == 0 || ($3 + 0) < worst) {worst=$3 + 0; gen=$1; seen=1} END {printf "%s %.12g", gen, worst}' "$convergence_csv")"
    best_generation="$(echo "$best_pair" | awk '{print $1}')"
    best_score="$(echo "$best_pair" | awk '{print $2}')"
    worst_generation="$(echo "$worst_pair" | awk '{print $1}')"
    worst_score="$(echo "$worst_pair" | awk '{print $2}')"

    echo "$run,$seed,$score,$runtime,$final_generation,$best_generation,$best_score,$worst_generation,$worst_score,$convergence_csv,$log_file" >> "$RUN_CSV"
done

echo ""
echo "Reliability runs complete."
echo "Run CSV: $RUN_CSV"
echo "Plot with:"
echo "  python3 scripts/plot_algo4_reliability.py $RUN_CSV -o $OUT_DIR/algo4_reliability.png"

if command -v python3 >/dev/null 2>&1; then
    echo "Generating plot: $PLOT_OUT"
    python3 "$REPO_ROOT/scripts/plot_algo4_reliability.py" "$RUN_CSV" -o "$PLOT_OUT" \
        --title "$PLOT_TITLE" --parameters "$PLOT_PARAMETERS" >/dev/null 2>&1 || \
        echo "warning: plotting failed; run the command above manually" >&2
fi
