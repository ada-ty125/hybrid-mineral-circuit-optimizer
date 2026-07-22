#!/usr/bin/env python3
"""
Plot Algorithm 4 reliability across independent seeded runs.

Usage:
  python3 scripts/plot_algo4_reliability.py \
      build/ga_reliability_algo4/algo4_reliability_runs.csv \
      -o build/ga_reliability_algo4/algo4_reliability.png
"""

from __future__ import annotations

import argparse
import csv
import math
import os
import statistics
from pathlib import Path

cache_dir = Path.cwd() / "build" / "matplotlib-cache"
cache_dir.mkdir(parents=True, exist_ok=True)
os.environ.setdefault("MPLCONFIGDIR", str(cache_dir))
os.environ.setdefault("XDG_CACHE_HOME", str(cache_dir))

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt

DEFAULT_PARAMETER_TEXT = (
    "population_size=362   max_iterations=988   crossover_probability=0.9350\n"
    "mutation_probability=0.2142   tournament_size=2   early_stop_patience=330\n"
    "num_crossover_points=1   elite_count=5   gaussian_sigma=0.2709"
)


def to_float(value: object, default: float | None = None) -> float | None:
    try:
        return float(value)
    except (TypeError, ValueError):
        return default


def read_run_metadata(csv_path: str) -> list[dict[str, str]]:
    with open(csv_path, newline="") as handle:
        rows = list(csv.DictReader(handle))
    good_rows = []
    for row in rows:
        if to_float(row.get("Score")) is None:
            continue
        convergence_path = row.get("ConvergenceCsv", "")
        if convergence_path and Path(convergence_path).is_file():
            good_rows.append(row)
    if not good_rows:
        raise ValueError(
            "No successful reliability runs with convergence CSVs were found."
        )
    return good_rows


def read_curve(path: str) -> dict[int, float]:
    curve: dict[int, float] = {}
    with open(path, newline="") as handle:
        reader = csv.DictReader(handle)
        for row in reader:
            generation = to_float(row.get("Generation"))
            value = to_float(row.get("GlobalBest"))
            if generation is None or value is None:
                continue
            curve[int(generation)] = value
    return curve


def forward_fill_curves(
    curves: list[dict[int, float]],
) -> tuple[list[int], list[list[float]]]:
    max_generation = max(max(curve) for curve in curves if curve)
    generations = list(range(max_generation + 1))
    filled = []

    for curve in curves:
        values = []
        last = None
        for generation in generations:
            if generation in curve:
                last = curve[generation]
            if last is None:
                last = min(curve.values())
            values.append(last)
        filled.append(values)

    return generations, filled


def style() -> None:
    plt.rcParams.update(
        {
            "font.family": "DejaVu Sans",
            "figure.facecolor": "#f7f8fb",
            "axes.facecolor": "#ffffff",
            "axes.edgecolor": "#d7dce5",
            "axes.grid": True,
            "grid.color": "#e7eaf0",
            "grid.linewidth": 0.8,
            "axes.titlesize": 18,
            "axes.labelsize": 12,
            "xtick.labelsize": 10,
            "ytick.labelsize": 10,
            "legend.fontsize": 10,
        }
    )


def plot_reliability(
    run_csv: str,
    out_path: str,
    title: str,
    parameter_text: str,
) -> None:
    rows = read_run_metadata(run_csv)
    curves = [read_curve(row["ConvergenceCsv"]) for row in rows]
    generations, filled = forward_fill_curves(curves)

    columns = list(zip(*filled))
    mean_curve = [statistics.mean(values) for values in columns]
    std_curve = [
        statistics.pstdev(values) if len(values) > 1 else 0.0 for values in columns
    ]
    lower = [mean - std for mean, std in zip(mean_curve, std_curve)]
    upper = [mean + std for mean, std in zip(mean_curve, std_curve)]

    final_scores = [to_float(row["Score"], 0.0) or 0.0 for row in rows]
    best_index = max(range(len(final_scores)), key=lambda i: final_scores[i])
    worst_index = min(range(len(final_scores)), key=lambda i: final_scores[i])
    best_run = rows[best_index]
    worst_run = rows[worst_index]
    best_curve = filled[best_index]
    worst_curve = filled[worst_index]

    mean_final = statistics.mean(final_scores)
    std_final = statistics.pstdev(final_scores) if len(final_scores) > 1 else 0.0
    best_final = final_scores[best_index]
    worst_final = final_scores[worst_index]
    mean_runtime = statistics.mean(
        [to_float(row.get("RuntimeSeconds"), 0.0) or 0.0 for row in rows]
    )
    run_count = len(rows)

    best_generation = int(
        to_float(best_run.get("BestGeneration"), len(generations) - 1) or 0
    )
    worst_generation = int(
        to_float(worst_run.get("BestGeneration"), len(generations) - 1) or 0
    )

    style()
    fig, ax = plt.subplots(figsize=(16, 8.6))

    band_color = "#7aa6c2"
    mean_color = "#155c7a"
    best_color = "#2f9e44"
    worst_color = "#c44e52"

    band = ax.fill_between(
        generations,
        lower,
        upper,
        color=band_color,
        alpha=0.22,
        linewidth=0,
        label=f"+/-1 std dev band (std={std_final:.1f})",
    )
    mean_line = ax.plot(
        generations,
        mean_curve,
        color=mean_color,
        linewidth=3.0,
        label=f"mean={mean_final:.1f}",
    )[0]
    best_line = ax.plot(
        generations,
        best_curve,
        color=best_color,
        linewidth=2.4,
        linestyle="--",
        label=f"best run: seed {best_run['Seed']} (score={best_final:.1f})",
    )[0]
    worst_line = ax.plot(
        generations,
        worst_curve,
        color=worst_color,
        linewidth=2.4,
        linestyle="--",
        label=f"worst run: seed {worst_run['Seed']} (score={worst_final:.1f})",
    )[0]

    ax.scatter(
        [best_generation],
        [best_curve[best_generation]],
        color=best_color,
        s=90,
        zorder=5,
    )
    ax.annotate(
        f"best generation {best_generation}",
        xy=(best_generation, best_curve[best_generation]),
        xytext=(16, 14),
        textcoords="offset points",
        color=best_color,
        fontsize=10,
        weight="bold",
    )
    ax.scatter(
        [worst_generation],
        [worst_curve[worst_generation]],
        color=worst_color,
        s=90,
        zorder=5,
    )
    ax.annotate(
        f"worst-run best generation {worst_generation}",
        xy=(worst_generation, worst_curve[worst_generation]),
        xytext=(16, -22),
        textcoords="offset points",
        color=worst_color,
        fontsize=10,
        weight="bold",
    )

    ax.set_title(title, loc="left", weight="bold", pad=28)
    ax.set_xlabel("Generation")
    ax.set_ylabel("Global best score")
    ax.set_xlim(0, max(generations))
    ax.margins(y=0.08)

    ax.legend(
        handles=[band, mean_line, best_line, worst_line],
        loc="lower right",
        frameon=True,
        framealpha=0.94,
        title="Reliability summary",
        borderpad=0.9,
        labelspacing=0.75,
    )

    summary = (
        f"mean={mean_final:.1f}   std={std_final:.1f}\n"
        f"best={best_final:.1f}, {best_generation} generations\n"
        f"worst={worst_final:.1f}, {worst_generation} generations\n"
        f"runtime={mean_runtime:.2f}s"
    )
    ax.text(
        0.018,
        0.965,
        summary,
        transform=ax.transAxes,
        va="top",
        ha="left",
        fontsize=10.5,
        color="#111827",
        bbox={
            "boxstyle": "round,pad=0.6",
            "facecolor": "#ffffff",
            "edgecolor": "#d7dce5",
            "alpha": 0.96,
        },
    )
    fig.text(0.125, 0.905, parameter_text, fontsize=10.5, color="#4b5563")

    fig.tight_layout(rect=(0, 0, 1, 0.91))
    Path(out_path).parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(out_path, dpi=220, bbox_inches="tight")
    print(f"Saved reliability plot to: {out_path}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Plot Algorithm 4 reliability curves.")
    parser.add_argument(
        "run_csv", help="algo4_reliability_runs.csv from run_algo4_reliability.sh"
    )
    parser.add_argument(
        "-o",
        "--out",
        default="build/ga_reliability_algo4/algo4_reliability.png",
        help="Output image path",
    )
    parser.add_argument(
        "--title",
        default="GA reliability across different seeds",
        help="Chart title",
    )
    parser.add_argument(
        "--parameters",
        default=DEFAULT_PARAMETER_TEXT,
        help="Multiline parameter summary shown above the chart",
    )
    args = parser.parse_args()

    plot_reliability(args.run_csv, args.out, args.title, args.parameters)
