#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$REPO_ROOT/build_problem3"
OUT_DIR="$REPO_ROOT/output/problem3"

mkdir -p "$BUILD_DIR" "$OUT_DIR"

cmake -S "$REPO_ROOT" -B "$BUILD_DIR" \
    -DGA_SOURCE_FILE="$REPO_ROOT/src/Genetic_Algorithm.cpp"
cmake --build "$BUILD_DIR" --target Circuit_Optimizer -j

"$BUILD_DIR/bin/Circuit_Optimizer" \
    362 988 0.935 0.2142 2 330 1 1 0.2709 swapable "$OUT_DIR/problem3.png" \
    | tee "$OUT_DIR/problem3.log"
