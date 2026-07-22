#!/usr/bin/env python3
"""
Plot GA benchmark summaries produced by scripts/benchmark_ga_variants.sh.

Usage:
  python3 scripts/plot_ga_benchmark.py build/ga_benchmark/ga_benchmark_summary.csv \
      -o build/ga_benchmark/ga_benchmark_scores.png
"""

from __future__ import annotations

import argparse
import csv
import math
import os
import statistics
from collections import defaultdict
from pathlib import Path

cache_dir = Path.cwd() / "build" / "matplotlib-cache"
cache_dir.mkdir(parents=True, exist_ok=True)
os.environ.setdefault("MPLCONFIGDIR", str(cache_dir))
os.environ.setdefault("XDG_CACHE_HOME", str(cache_dir))

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt


SUMMARY_COLUMNS = {"Algorithm", "Parameter", "Value", "MeanScore", "StdDevScore"}
RAW_COLUMNS = {"Algorithm", "Parameter", "Value", "Score"}
PREFERRED_ALGORITHM_ORDER = ["Baseline", "NaiveOpenMP", "Algo3", "Algo4"]


def to_float(value: object, default: float | None = None) -> float | None:
    try:
        return float(value)
    except (TypeError, ValueError):
        return default


def read_rows(csv_path: str) -> list[dict[str, str]]:
    with open(csv_path, newline="") as handle:
        reader = csv.DictReader(handle)
        if not reader.fieldnames:
            raise ValueError(f"{csv_path} is empty. Try plotting ga_benchmark_runs.csv instead.")
        rows = list(reader)

    columns = set(reader.fieldnames)
    if SUMMARY_COLUMNS.issubset(columns):
        return [
            {
                "Algorithm": row["Algorithm"],
                "Parameter": row["Parameter"],
                "Value": row["Value"],
                "MeanScore": row["MeanScore"],
                "StdDevScore": row.get("StdDevScore", "0"),
                "MaxScore": row.get("MaxScore", row["MeanScore"]),
            }
            for row in rows
            if to_float(row.get("MeanScore")) is not None
        ]

    if RAW_COLUMNS.issubset(columns):
        grouped: dict[tuple[str, str, str], list[float]] = defaultdict(list)
        for row in rows:
            score = to_float(row.get("Score"))
            if score is None:
                continue
            grouped[(row["Algorithm"], row["Parameter"], row["Value"])].append(score)

        summary_rows = []
        for (algorithm, parameter, value), scores in grouped.items():
            stdev = statistics.pstdev(scores) if len(scores) > 1 else 0.0
            summary_rows.append(
                {
                    "Algorithm": algorithm,
                    "Parameter": parameter,
                    "Value": value,
                    "MeanScore": str(statistics.mean(scores)),
                    "StdDevScore": str(stdev),
                    "MaxScore": str(max(scores)),
                }
            )
        return summary_rows

    raise ValueError(
        "CSV must be either a summary CSV with Algorithm,Parameter,Value,MeanScore,StdDevScore "
        "or a raw CSV with Algorithm,Parameter,Value,Score."
    )


def stable_unique(values: list[str]) -> list[str]:
    seen = set()
    result = []
    for value in values:
        if value not in seen:
            seen.add(value)
            result.append(value)
    return result


def algorithm_order(rows: list[dict[str, str]]) -> list[str]:
    names = set(row["Algorithm"] for row in rows)
    ordered = [name for name in PREFERRED_ALGORITHM_ORDER if name in names]
    ordered.extend(sorted(names - set(ordered)))
    return ordered


def color_mapping(algorithms: list[str]) -> dict[str, str]:
    color_cycle = plt.rcParams["axes.prop_cycle"].by_key().get("color", ["C0", "C1", "C2", "C3"])
    return {algorithm: color_cycle[i % len(color_cycle)] for i, algorithm in enumerate(algorithms)}


def prepare_parameter_rows(rows: list[dict[str, str]], parameter: str) -> list[dict[str, object]]:
    part = [row.copy() for row in rows if row["Parameter"] == parameter]
    numeric_values = [to_float(row["Value"]) for row in part]

    if all(value is not None for value in numeric_values):
        for row, numeric in zip(part, numeric_values):
            row["ValueNumeric"] = numeric
            row["ValueLabel"] = row["Value"]
    else:
        labels = sorted({row["Value"] for row in part})
        label_order = {label: index for index, label in enumerate(labels)}
        for row in part:
            row["ValueNumeric"] = label_order[row["Value"]]
            row["ValueLabel"] = row["Value"]

    return sorted(part, key=lambda row: (str(row["Algorithm"]), float(row["ValueNumeric"])))


def style_plot() -> None:
    for style in ("seaborn-v0_8-darkgrid", "seaborn-darkgrid"):
        try:
            plt.style.use(style)
            return
        except Exception:
            pass


def plot_benchmark(csv_path: str, out_path: str, show: bool = False) -> None:
    rows = read_rows(csv_path)
    if not rows:
        raise ValueError("No plottable rows found.")

    parameters = stable_unique([row["Parameter"] for row in rows])
    algorithms = algorithm_order(rows)

    style_plot()
    cols = 2 if len(parameters) > 1 else 1
    plot_rows = math.ceil(len(parameters) / cols)
    fig, axes = plt.subplots(
        plot_rows,
        cols,
        figsize=(7.4 * cols + 2.2, 4.8 * plot_rows),
        squeeze=False,
    )

    colors = color_mapping(algorithms)

    for ax, parameter in zip(axes.flat, parameters):
        part = prepare_parameter_rows(rows, parameter)

        for algorithm in algorithms:
            series = [row for row in part if row["Algorithm"] == algorithm]
            if not series:
                continue

            x = [float(row["ValueNumeric"]) for row in series]
            y = [float(row["MeanScore"]) for row in series]
            err = [float(row.get("StdDevScore") or 0.0) for row in series]
            color = colors[algorithm]

            ax.plot(x, y, marker="o", linewidth=2.2, markersize=5, label=algorithm, color=color)
            ax.fill_between(
                x,
                [value - spread for value, spread in zip(y, err)],
                [value + spread for value, spread in zip(y, err)],
                color=color,
                alpha=0.18,
                linewidth=0,
            )

        tick_pairs = sorted(
            {(float(row["ValueNumeric"]), str(row["ValueLabel"])) for row in part},
            key=lambda item: item[0],
        )
        ax.set_xticks([pair[0] for pair in tick_pairs])
        ax.set_xticklabels([pair[1] for pair in tick_pairs], rotation=20, ha="right")
        ax.set_title(parameter.replace("_", " ").title())
        ax.set_xlabel("Parameter value")
        ax.set_ylabel("Mean score")
        ax.grid(True, alpha=0.35)

    for ax in axes.flat[len(parameters):]:
        ax.axis("off")

    handles, labels = axes.flat[0].get_legend_handles_labels()
    if handles:
        fig.legend(
            handles,
            labels,
            loc="center left",
            bbox_to_anchor=(0.86, 0.52),
            frameon=True,
            title="Algorithm",
            borderpad=0.8,
            labelspacing=0.8,
        )
        fig.subplots_adjust(right=0.84, top=0.92)

    fig.suptitle("GA Benchmark: Mean Score With Std-Dev Band", fontsize=16, y=0.995)
    fig.tight_layout(rect=(0, 0, 0.84, 0.94))

    Path(out_path).parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(out_path, dpi=180, bbox_inches="tight")
    print(f"Saved plot to: {out_path}")

    if show:
        plt.show()


def plot_iteration_best(csv_path: str, out_path: str, show: bool = False) -> None:
    rows = read_rows(csv_path)
    if not rows:
        raise ValueError("No plottable rows found.")

    algorithms = algorithm_order(rows)
    colors = color_mapping(algorithms)
    part = prepare_parameter_rows(rows, "max_iterations")
    if not part:
        raise ValueError("No max_iterations rows found in CSV.")

    style_plot()
    fig, ax = plt.subplots(figsize=(14.5, 6.2))

    for algorithm in algorithms:
        series = [row for row in part if row["Algorithm"] == algorithm]
        if not series:
            continue

        x = [float(row["ValueNumeric"]) for row in series]
        y = [float(row.get("MaxScore") or row["MeanScore"]) for row in series]
        err = [float(row.get("StdDevScore") or 0.0) for row in series]
        color = colors[algorithm]
        lower = [value - spread for value, spread in zip(y, err)]
        upper = [value + spread for value, spread in zip(y, err)]

        ax.plot(x, y, marker="o", linewidth=2.6, markersize=6, label=algorithm, color=color)
        ax.plot(x, upper, linestyle="--", linewidth=1.2, color=color, alpha=0.75)
        ax.plot(x, lower, linestyle="--", linewidth=1.2, color=color, alpha=0.75)
        ax.fill_between(
            x,
            lower,
            upper,
            color=color,
            alpha=0.13,
            linewidth=0,
        )

    tick_pairs = sorted(
        {(float(row["ValueNumeric"]), str(row["ValueLabel"])) for row in part},
        key=lambda item: item[0],
    )
    ax.set_xticks([pair[0] for pair in tick_pairs])
    ax.set_xticklabels([pair[1] for pair in tick_pairs])
    ax.set_xlabel("Max iterations")
    ax.set_ylabel("Best score across repeats")
    ax.set_title("Best Score vs Max Iterations")
    ax.grid(True, alpha=0.35)

    ax.legend(
        loc="lower right",
        frameon=True,
        title="Algorithm",
        borderpad=0.8,
        labelspacing=0.7,
        framealpha=0.92,
    )
    fig.tight_layout()

    Path(out_path).parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(out_path, dpi=180, bbox_inches="tight")
    print(f"Saved iteration plot to: {out_path}")

    if show:
        plt.show()


def default_iteration_out(out_path: str) -> str:
    path = Path(out_path)
    return str(path.with_name(f"{path.stem}_iterations{path.suffix or '.png'}"))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Plot GA benchmark mean score and std-dev bands.")
    parser.add_argument("csv", help="Summary or raw CSV from benchmark_ga_variants.sh")
    parser.add_argument(
        "-o",
        "--out",
        default="build/ga_benchmark/ga_benchmark_scores.png",
        help="Output image path",
    )
    parser.add_argument(
        "--iteration-out",
        help="Output image path for best score vs max iterations. Defaults to OUT stem + _iterations.png",
    )
    parser.add_argument(
        "--no-iteration-plot",
        action="store_true",
        help="Only generate the all-parameters plot",
    )
    parser.add_argument("--show", action="store_true", help="Show the plot interactively")
    args = parser.parse_args()

    plot_benchmark(args.csv, args.out, args.show)
    if not args.no_iteration_plot:
        plot_iteration_best(args.csv, args.iteration_out or default_iteration_out(args.out), args.show)
