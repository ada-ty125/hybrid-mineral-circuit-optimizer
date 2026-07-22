# Contribution record

This repository is a portfolio edition of a group assignment, not a claim of sole authorship.

## Tianyu Yang's scope

Tianyu led the genetic-algorithm workstream and contributed:

- the core GA framework and mock-based verification;
- hybrid discrete/continuous optimization and tournament selection;
- OpenMP parallel evaluation, thread-local random-number generation, and early stopping;
- later restructuring of the breeding loop, independent child validity capture, explorer injection,
  adaptive mutation shock, and incremental offspring-only evaluation;
- deterministic seed/convergence controls and the benchmark/reliability tooling included in this
  portfolio edition.

Representative commits retained in the repository history:

| Commit | Date | Evidence |
|---|---|---|
| `cb7158e` | 2026-05-18 | Core genetic algorithm framework and mock verification |
| `6231806` | 2026-05-18 | OpenMP, early stopping, hybrid optimization, tournament selection |
| `8d0a475` | 2026-05-18 | GA tests |
| `ad92676` | 2026-05-19 | Circuit/unit integration work |
| `46341a9` | 2026-05-20 | Major GA algorithm update |
| `dde75eb` | 2026-05-20 | Formatting and integration cleanup |

The current `src/Genetic_Algorithm.cpp` also contains collaborative work by Hongxin Chen. Use
`git blame src/Genetic_Algorithm.cpp` and `git log -- src/Genetic_Algorithm.cpp` for line-level and
commit-level attribution.

## Team scope

The Cuprite Team members named in the original license are:

- Tiankai Bu
- Tianyu Yang
- Shuning Lin
- Julia J Williams
- Nitya Gomathi Vivekananthan
- Hongxin Chen
- Shaolong Chen
- Ju Lin

Across the project, the team produced the circuit simulator, economic model, graph validity logic,
visualization, CLI/integration work, tests, documentation, and GA engine. The Git history is
preserved rather than squashed so a reviewer can distinguish these contributions.

## Portfolio-only additions

The top-level portfolio documentation, benchmark evidence packaging, reproducibility environment
controls, and public-repository presentation were added after the assessed group submission. These
additions do not change the authorship of the underlying team components.

## License and upstream

The code is available under the original Cuprite Team MIT License. The upstream course repository
is recorded as the `upstream` Git remote in local clones of this portfolio edition.
