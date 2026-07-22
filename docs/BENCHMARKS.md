# Benchmark methodology

This document defines what the checked-in performance claims mean and points to the underlying
evidence.

## Test matrix

The harness compares four implementations:

| Label | Description |
|---|---|
| Baseline | Serial/reference GA |
| NaiveOpenMP | Direct OpenMP parallelization |
| Algo3 | Parallel hybrid GA before the final constraint fallbacks |
| Algo4 | Current hybrid GA with independent child capture and explorer fallback |

For each implementation, the harness sweeps population size, iteration cap, crossover probability,
and mutation probability. Each setting is repeated five times. Except for the parameter being
swept, defaults are population `100`, iterations `200`, crossover `0.9`, mutation `0.01`, tournament
size `3`, patience `50`, crossover points `2`, and elite count `1` in fixed-topology mode.

## Portfolio headline comparison

The README compares the `Pop_Size_2000` rows because both implementations use the same population,
iteration cap, objective, and repeat count.

| Statistic | Baseline | Algo4 |
|---|---:|---:|
| Successful runs | 5/5 | 5/5 |
| Mean score | 323.3422 | 770.1312 |
| Population standard deviation | 32.4751 | 36.6731 |
| Minimum score | 282.441 | 721.321 |
| Maximum score | 361.929 | 812.925 |
| Mean runtime | 261.8664 s | 58.8407 s |
| Runtime standard deviation | 13.9207 s | 6.6446 s |

Derived values:

- score ratio: `770.1312 / 323.3422 = 2.382x`;
- score improvement: `+138.2%`;
- runtime speedup: `261.8664 / 58.8407 = 4.45x`;
- runtime reduction: `77.5%`.

The best observed score across the full study is `812.925`. The public documentation does not claim
that every seed reaches this value.

## Environment

The checked-in run was produced on:

- macOS 26.5.2, Darwin 25.5.0, arm64;
- Apple Clang 17.0.0;
- CMake 4.3.2;
- OpenMP-enabled C++20 build;
- no explicit `CMAKE_BUILD_TYPE` in the archived run.

Runtime comparisons are meaningful within this single-machine experiment. They should be rerun
before making claims about another CPU, compiler, thread count, or build type.

## Evidence files

- [ga_benchmark_runs.csv](../benchmarks/data/ga_benchmark_runs.csv): one row per run;
- [ga_benchmark_summary.csv](../benchmarks/data/ga_benchmark_summary.csv): grouped means,
  deviations, extrema, and runtimes;
- [ga_tuning_results.csv](../benchmarks/data/ga_tuning_results.csv): a single-run tuning sweep;
- [benchmark variants](../benchmarks/variants): the compared C++ implementations;
- [benchmark harness](../scripts/benchmark_ga_variants.sh): build, run, and aggregation logic.

## Reproduction

Compile and smoke-test every implementation:

```bash
./scripts/benchmark_ga_variants.sh \
  --cpp Baseline:benchmarks/variants/Genetic_Algorithmbaseline.cpp \
  --cpp NaiveOpenMP:benchmarks/variants/Genetic_AlgorithmNaiveOPenMP.cpp \
  --cpp Algo3:benchmarks/variants/Genetic_AlgorithmAlgo3.cpp \
  --cpp Algo4:src/Genetic_Algorithm.cpp \
  --check-only
```

Remove `--check-only` and add `--repeats 5` for the full sweep. It is computationally expensive.
Use `GA_SEED` for deterministic single-implementation runs. OpenMP scheduling can still affect exact
cross-platform reproducibility.

## Limitations and claim hygiene

- Score and runtime are reported together; one should not be read without the other.
- A maximum from one group is never presented as the mean of that group.
- Standard deviation describes across-seed variability, not convergence within one run.
- The theoretical saving from not reevaluating parents is distinct from wall-clock speedup.
- The original resume draft's `140.94 -> 23.06` variance claim is intentionally omitted because the
  corresponding raw experiment is not present in the workspace evidence.
