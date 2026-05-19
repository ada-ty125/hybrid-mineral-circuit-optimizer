# Development Guide

This document is the shared development contract for the Cuprite team. If
there is disagreement between module implementations, this document is the
first place to check. Changes to the public interfaces or circuit vector format
must be agreed by the team before implementation.

## Project Goals

The project target is a complete optimisation tool, not only a minimum working
demo. The final software should:

- Optimise the fixed 10-unit base case with 7 type A units and 3 type B units.
- Optimise the swappable 8-unit case, including both unit type choices and
  circuit connections.
- Report economic performance, recoveries, grades, operating cost, seed,
  runtime, and convergence information.
- Generate a visualisation of the best circuit.
- Support repeated runs and parameter scans for robustness analysis.
- Build and test through GitHub Actions on every branch.

## Team Ownership

Ownership is not yet assigned. Each task is expected to have two members.
| Task | Scope | Owners |
|---|---|---|
| Genetic Algorithm | Genetic Algorithm |  |
| Circuit Simulator | Circuit Simulator |  |
| Validity Checker | Validity Checker |  |
| Visualisation | Visualisation |  |

Ownership means first responsibility for design and review. It does not prevent
other team members from contributing.

## Source Of Truth

- The required public interfaces are in `include/RequiredFunctions.h`.
- The physical and economic model is defined by
  `Problem Statement for Genetic Algorithms Project 2026.pdf`.
- This file defines the team-level implementation standards.

Do not change `include/RequiredFunctions.h` unless the change is reviewed by
the team manager and communicated to all module owners.

## Circuit Vector Standard

All modules must use the same `std::span<const int>` circuit representation:

```text
[
  num_inputs,
  num_units,
  num_products,
  unit0_outputs,
  unit1_outputs,
  ...,
  unitN_outputs,
  feed_dest,
  unit0_output0_dest,
  unit0_output1_dest,
  [unit0_output2_dest],
  unit1_output0_dest,
  unit1_output1_dest,
  [unit1_output2_dest],
  ...
]
```

For the base project:

- `num_inputs` is `1`.
- `num_products` is `3`.
- Each `unit_outputs[i] == 2` means unit `i` is type A.
- Each `unit_outputs[i] == 3` means unit `i` is type B.
- Unit destination IDs are `0` to `num_units - 1`.
- Product destination IDs are:
  - `num_units`: palusznium concentrate stream.
  - `num_units + 1`: gormanium concentrate stream.
  - `num_units + 2`: final tailings stream.
- `feed_dest` must be a unit ID, never a product stream ID.

The vector length must be:

```text
3 + num_units + 1 + sum(unit_outputs)
```

The fixed 10-unit base case must have 7 type A units and 3 type B units, with
any ordering. For example:

```text
num_units = 10
unit_outputs = [2, 2, 2, 2, 2, 2, 2, 3, 3, 3]
```

The swappable 8-unit case should use:

```text
num_units = 8
each unit_outputs[i] is either 2 or 3, chosen by the genetic algorithm
```

## Validity Standard

`check_validity(std::span<const int> circuit_span)` must be fast and must not
run the full mass-balance simulation.

A circuit is invalid if any of the following are true:

- The vector length is not exactly `3 + num_units + 1 + sum(unit_outputs)`.
- Any unit output count is not `2` or `3`.
- `feed_dest` is not in the inclusive range `[0, num_units - 1]`.
- Any stream destination is not in the inclusive range `[0, num_units + 2]`.
- Any unit sends an output stream directly to itself.
- All output streams from one unit have the same destination.
- At least one unit is not reachable from the feed.
- At least one unit cannot reach at least two of the outlet streams,
  where the outlets are the palusznium concentrate, the gormanium concentrate,
  and the final tailings (`num_units`, `num_units + 1`, `num_units + 2`).

The simulator must still detect non-convergence, because validity checks may
not catch every pathological circuit.

## Simulator Standard

`circuit_performance(std::span<const int> circuit_span)` returns a value to be
maximised by the genetic algorithm.

Use the physical constants from the project PDF:

- Feed:
  - Palusznium: `8 kg/s`
  - Gormanium: `12 kg/s`
  - Waste: `80 kg/s`
- Unit volume: `10 m3`
- Solids volume fraction: `0.1`
- Solid density: `3000 kg/m3`
- Type A operating cost: `10 GBP/s`
- Type B operating cost: `12 GBP/s`

Use successive substitution for mass-balance convergence:

- Default tolerance: `1e-6`.
- Default maximum iterations: at least `1000`, adjustable through parameters.
- Apply a minimum volumetric flow-rate of `1e-10 m^3/s` to avoid overflow in
  residence-time calculations, as suggested by the project PDF.
- If the circuit fails to converge, return the worst-case fitness suggested by
  the project PDF, which is the waste feed rate multiplied by the per-kg waste
  penalty in a concentrate stream, and record the failure in diagnostics when
  possible.

The returned performance is the net revenue per second, computed from the
per-kg rates given in the project PDF appendix. It must include:

- For each kg/s in the palusznium concentrate stream: a positive payment for
  palusznium and a negative penalty for gormanium and for waste.
- For each kg/s in the gormanium concentrate stream: a positive payment for
  gormanium and a negative penalty for palusznium and for waste.
- The final tailings stream is discarded at no cost or revenue.
- A subtracted operating cost of `10 GBP/s` per type A unit and `12 GBP/s` per
  type B unit.

## Genetic Algorithm Standard

The GA must treat higher performance as better.

The GA implementation should support:

- Population initialisation.
- Valid individual generation.
- Fitness evaluation through `performance_function`.
- Fast rejection through `validity_checker`.
- Elitism, preserving the best individual each generation.
- Parent selection.
- Crossover.
- Mutation.
- Convergence stopping.
- Multiple random seeds.

Default GA parameters should be centralised, not scattered through the code:

- Population size.
- Maximum generations.
- Crossover probability.
- Mutation probability.
- Maximum mutations per child.
- Stagnation limit.
- Random seed.

Recommended initial values:

```text
population_size = 200
max_generations = 1000
crossover_probability = 0.9
mutation_probability = 0.01
max_mutations_per_child = team-chosen, tune with sensitivity runs
stagnation_limit = 100
random_seed = team-chosen, but recorded with every run
```

These are starting points only. Final results must include parameter
sensitivity analysis.

## Visualisation Standard

The visualisation module should convert a circuit vector into a directed graph.

Required output:

- At least one graph image for the best fixed 10-unit circuit.
- At least one graph image for the best swappable 8-unit circuit.
- Type A and type B units should be visually distinguishable.
- Product streams and tailings should be labelled.

Preferred output formats:

- DOT file for reproducibility.
- PNG or PDF image for presentation.

## Result Reporting Standard

Each optimisation run should record:

- Case name.
- Circuit vector.
- Best performance.
- Palusznium recovery.
- Palusznium grade.
- Gormanium recovery.
- Gormanium grade.
- Operating cost.
- Population size.
- Mutation probability.
- Crossover probability.
- Random seed.
- Number of generations.
- Runtime.
- Whether convergence was reached.

CSV is the preferred format for repeated run and parameter scan results.

## Branching Workflow

Use the following shared branches:

- `main`
- `main_backup`
- `ga`
- `simulator`
- `validity-checker`
- `visualisation`

Branch rules:

- `main` is the final stable branch.
- `main_backup` is the integration branch.
- `ga` is for genetic algorithm development.
- `simulator` is for circuit simulator development.
- `validity-checker` is for graph validity checking development.
- `visualisation` is for visualisation, integration scripts, documentation, and
  analysis outputs.
- Development branches must open pull requests into `main_backup`.
- `main` must only be updated through pull requests from `main_backup`.
- Do not merge a PR that breaks GitHub Actions.
- Do not modify another group's module without coordination.

## Commit Message Standard

Use a short conventional-commit style message for every commit. The team uses
uppercase commit types and a file or module scope:

```text
TYPE(scope): short imperative description
```

Allowed commit types:

- `FEAT(xxx.file): ...` for new features.
- `FIX(xxx.file): ...` for bug fixes.
- `DOCS(xxx.file): ...` for documentation or docstring updates.
- `CHORE(xxx.file): ...` for maintenance or non-functional changes.

Examples:

```text
FEAT(Genetic_Algorithm.cpp): add elitist parent selection
FIX(CSimulator.cpp): handle zero-flow residence time
DOCS(DEVELOPMENT.md): add branch workflow
CHORE(run_tests.yml): run CI on all branches
```

Keep the description short, clear, and consistent. Do not end the subject line
with a period.

## Pull Request Checklist

Every PR should include:

- What changed.
- Which files were touched.
- How it was tested.
- Whether public interfaces changed.
- Whether the circuit vector standard changed.
- Any known limitations.

## Testing Standard

Each module owner must add tests for their module.

Required tests:

- GA converges on a toy problem with a known optimum.
- PRNG stays within requested bounds.
- Validity checker rejects malformed vectors.
- Validity checker rejects self-recycle.
- Validity checker rejects unreachable units.
- Simulator gives expected output for simple manually checked circuits.
- Simulator handles non-convergent or invalid circuits safely.
- The final executable runs without interactive input.

GitHub Actions runs on every branch push and pull request.

## Four And A Half Day Plan

Day 0.5:

- Freeze this development standard.
- Confirm branch ownership.
- Add basic README build and run instructions.

Day 1:

- GA runs on a toy problem.
- Simulator runs on simple hand-written circuits.
- Validity checker handles basic graph checks.
- Visualisation produces a basic graph.

Day 2:

- Integrate GA, simulator, and validity checker.
- Run the fixed 10-unit case.
- Add tests for invalid circuits and simple simulator cases.

Day 3:

- Run the swappable 8-unit case.
- Add repeated seed runs.
- Add GA parameter scans.
- Produce first best-circuit plots.

Day 4:

- Run economic and feed sensitivity experiments.
- Improve performance and reliability.
- Finish README and presentation result tables.

Final half day:

- Bug fixes only.
- Freeze final outputs.
- Ensure GitHub Actions passes.
- Prepare presentation evidence and final summary.
