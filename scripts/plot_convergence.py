#!/usr/bin/env python3
"""
Plot convergence data from a CSV file with columns: Generation,CurrentBest,GlobalBest
Usage:
  python3 scripts/plot_convergence.py build/convergence.csv -o build/convergence.png
"""

import argparse
import sys

try:
    import pandas as pd
except Exception:
    print(
        "This script requires pandas. Install with: pip install pandas matplotlib",
        file=sys.stderr,
    )
    raise

import matplotlib.pyplot as plt


def plot_convergence(csv_path: str, out_path: str | None, show: bool = False):
    df = pd.read_csv(csv_path)
    if "Generation" not in df.columns:
        # try to infer first column as Generation
        df = df.rename(columns={df.columns[0]: "Generation"})

    x = df["Generation"]
    # pick columns that exist
    series = []
    for name in ("CurrentBest", "GlobalBest"):
        if name in df.columns:
            series.append((name, df[name]))

    if not series:
        raise ValueError("No CurrentBest or GlobalBest columns found in CSV")

    try:
        plt.style.use("seaborn-darkgrid")
    except Exception:
        pass
    plt.figure(figsize=(10, 6))
    for name, y in series:
        plt.plot(x, y, label=name)

    plt.xlabel("Generation")
    plt.ylabel("Value")
    plt.title("Convergence")
    plt.legend()
    plt.grid(True)
    plt.tight_layout()

    if out_path:
        plt.savefig(out_path, dpi=150)
        print(f"Saved plot to: {out_path}")
    if show and not out_path:
        plt.show()


if __name__ == "__main__":
    p = argparse.ArgumentParser(description="Plot convergence CSV")
    p.add_argument("csv", help="Path to CSV file")
    p.add_argument(
        "-o",
        "--out",
        help="Output image path (PNG, PDF, etc.)",
        default="build/convergence.png",
    )
    p.add_argument("--show", action="store_true", help="Show the plot interactively")
    args = p.parse_args()

    plot_convergence(args.csv, args.out, args.show)
