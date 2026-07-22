#!/usr/bin/env python3
"""
Create a presentation-ready Algorithm 4 benchmark figure.

Usage:
  python3 scripts/plot_algo4_report.py build/ga_benchmark/ga_benchmark_summary.csv \
      -o build/ga_benchmark/algo4_business_report.png
"""

from __future__ import annotations

import argparse
import csv
import os
from pathlib import Path

cache_dir = Path.cwd() / "build" / "matplotlib-cache"
cache_dir.mkdir(parents=True, exist_ok=True)
os.environ.setdefault("MPLCONFIGDIR", str(cache_dir))
os.environ.setdefault("XDG_CACHE_HOME", str(cache_dir))

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt


PARAMETERS = [
    ("population_size", "Population size", "log"),
    ("max_iterations", "Max iterations", "log"),
    ("crossover_probability", "Crossover probability", "linear"),
    ("mutation_probability", "Mutation probability", "log"),
]


def as_float(value: str, default: float = 0.0) -> float:
    try:
        return float(value)
    except (TypeError, ValueError):
        return default


def read_algo_rows(csv_path: str, algorithm: str) -> list[dict[str, str]]:
    with open(csv_path, newline="") as handle:
        rows = list(csv.DictReader(handle))
    return [row for row in rows if row.get("Algorithm") == algorithm]


def rows_for_parameter(rows: list[dict[str, str]], parameter: str) -> list[dict[str, str]]:
    return sorted(
        [row for row in rows if row.get("Parameter") == parameter],
        key=lambda row: as_float(row.get("Value", "0")),
    )


def best_row(rows: list[dict[str, str]]) -> dict[str, str]:
    return max(rows, key=lambda row: as_float(row.get("MaxScore", row.get("MeanScore", "0"))))


def fastest_row(rows: list[dict[str, str]]) -> dict[str, str]:
    return min(rows, key=lambda row: as_float(row.get("MeanRuntimeSeconds", "inf"), float("inf")))


def style() -> None:
    plt.rcParams.update(
        {
            "font.family": "DejaVu Sans",
            "axes.titlesize": 13,
            "axes.labelsize": 10,
            "xtick.labelsize": 9,
            "ytick.labelsize": 9,
            "legend.fontsize": 9,
            "figure.facecolor": "#f7f8fb",
            "axes.facecolor": "#ffffff",
            "axes.edgecolor": "#d6dae2",
            "axes.grid": True,
            "grid.color": "#e6e9ef",
            "grid.linewidth": 0.8,
            "grid.alpha": 1.0,
        }
    )


def plot_algo4_report(csv_path: str, out_path: str, algorithm: str = "Algo4") -> None:
    rows = read_algo_rows(csv_path, algorithm)
    if not rows:
        raise ValueError(f"No rows found for algorithm {algorithm!r}")

    style()

    score_color = "#c44e52"
    best_color = "#7b1e2b"
    runtime_color = "#4b5563"

    fig, axes = plt.subplots(2, 2, figsize=(16, 9), constrained_layout=False)
    axes = axes.flat

    for ax, (parameter, title, scale) in zip(axes, PARAMETERS):
        series = rows_for_parameter(rows, parameter)
        if not series:
            ax.axis("off")
            continue

        x = [as_float(row["Value"]) for row in series]
        mean_score = [as_float(row["MeanScore"]) for row in series]
        std_score = [as_float(row["StdDevScore"]) for row in series]
        max_score = [as_float(row.get("MaxScore", row["MeanScore"])) for row in series]
        runtime = [as_float(row.get("MeanRuntimeSeconds", "0")) for row in series]
        labels = [row["Value"] for row in series]

        lower = [score - spread for score, spread in zip(mean_score, std_score)]
        upper = [score + spread for score, spread in zip(mean_score, std_score)]

        ax.fill_between(x, lower, upper, color=score_color, alpha=0.16, linewidth=0)
        ax.plot(x, mean_score, color=score_color, marker="o", linewidth=2.6, label="Mean score")
        ax.plot(x, max_score, color=best_color, marker="D", linewidth=1.8, linestyle="--", label="Best score")

        best_idx = max(range(len(max_score)), key=lambda i: max_score[i])
        ax.scatter([x[best_idx]], [max_score[best_idx]], s=90, color=best_color, zorder=5)
        ax.annotate(
            f"best {max_score[best_idx]:.0f}",
            xy=(x[best_idx], max_score[best_idx]),
            xytext=(8, 12),
            textcoords="offset points",
            color=best_color,
            fontsize=9,
            weight="bold",
        )

        runtime_ax = ax.twinx()
        runtime_ax.plot(
            x,
            runtime,
            color=runtime_color,
            marker="s",
            linewidth=1.7,
            linestyle=":",
            label="Mean runtime",
        )
        runtime_ax.set_ylabel("Runtime (s)", color=runtime_color)
        runtime_ax.tick_params(axis="y", colors=runtime_color)
        runtime_ax.spines["right"].set_color("#d6dae2")

        if scale == "log":
            ax.set_xscale("log")
            runtime_ax.set_xscale("log")

        ax.set_title(title, loc="left", weight="bold")
        ax.set_xlabel("Parameter value")
        ax.set_ylabel("Score")
        ax.set_xticks(x)
        ax.set_xticklabels(labels)
        ax.margins(x=0.08)

        lines, line_labels = ax.get_legend_handles_labels()
        runtime_lines, runtime_labels = runtime_ax.get_legend_handles_labels()
        ax.legend(lines + runtime_lines, line_labels + runtime_labels, loc="lower right", framealpha=0.92)

    overall_best = best_row(rows)
    overall_fastest = fastest_row(rows)
    fig.suptitle(
        "Algorithm 4 Benchmark Performance",
        fontsize=24,
        weight="bold",
        x=0.04,
        y=0.975,
        ha="left",
    )
    fig.text(
        0.04,
        0.925,
        (
            "Mean score with standard-deviation band, best repeat score, and average runtime "
            "across each parameter sweep"
        ),
        fontsize=12,
        color="#4b5563",
    )
    fig.text(
        0.76,
        0.95,
        (
            f"Best observed: {as_float(overall_best.get('MaxScore', '0')):.1f} "
            f"({overall_best['Experiment']})\n"
            f"Fastest setting: {as_float(overall_fastest.get('MeanRuntimeSeconds', '0')):.2f}s "
            f"({overall_fastest['Experiment']})"
        ),
        fontsize=10,
        color="#111827",
        bbox={
            "boxstyle": "round,pad=0.55",
            "facecolor": "#ffffff",
            "edgecolor": "#d6dae2",
            "alpha": 0.96,
        },
    )

    fig.tight_layout(rect=(0.035, 0.04, 0.985, 0.89), h_pad=2.2, w_pad=2.4)
    Path(out_path).parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(out_path, dpi=220, bbox_inches="tight")
    print(f"Saved Algorithm 4 report figure to: {out_path}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Create a presentation-ready Algorithm 4 figure.")
    parser.add_argument("csv", help="Benchmark summary CSV")
    parser.add_argument(
        "-o",
        "--out",
        default="build/ga_benchmark/algo4_business_report.png",
        help="Output image path",
    )
    parser.add_argument("--algorithm", default="Algo4", help="Algorithm name in the CSV")
    args = parser.parse_args()

    plot_algo4_report(args.csv, args.out, args.algorithm)
