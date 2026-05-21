#!/usr/bin/env python3
"""Render a mineral-processing circuit animation from a JSON payload.

The C++ visualisation code writes the simulation payload and calls this script
for `.gif` outputs. Keeping the drawing here makes the layout and animation
easy to iterate without adding plotting dependencies to the simulator itself.
"""

from __future__ import annotations

import json
import math
import sys
from collections import defaultdict, deque
from pathlib import Path

import matplotlib

matplotlib.use("Agg")

import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation, PillowWriter
from matplotlib.patches import Circle, FancyArrowPatch, FancyBboxPatch
from matplotlib.path import Path as MplPath
import matplotlib.patheffects as path_effects


BACKGROUND = "#f6f8fb"
INK = "#172033"
MUTED = "#5b6475"
EDGE = "#97a2b6"
PANEL_EDGE = "#d6dce8"
PANEL_X = 0.805
PANEL_Y = 0.055
PANEL_WIDTH = 0.172
PANEL_HEIGHT = 0.855
NODE_LABEL_SIZE = 12
NODE_SUBLABEL_SIZE = 10
PRODUCT_LABEL_SIZE = 12
PANEL_BODY_SIZE = 10
PANEL_LIST_SIZE = 10


def flow_sum(values: list[float]) -> float:
    return float(sum(values))


def component_at(values: list[float], index: int) -> float:
    return float(values[index]) if index < len(values) else 0.0


def spaced_ys(count: int, top: float = 0.82, bottom: float = 0.18) -> list[float]:
    if count <= 1:
        return [(top + bottom) / 2.0]
    step = (top - bottom) / float(count - 1)
    return [top - i * step for i in range(count)]


def unit_number(node_id: str) -> int:
    try:
        return int(node_id.split("_", 1)[1])
    except (IndexError, ValueError):
        return 0


def build_layout(data: dict) -> tuple[dict[str, tuple[float, float]], dict[str, dict]]:
    nodes = {node["id"]: node for node in data["nodes"]}
    unit_ids = [node["id"] for node in data["nodes"] if node["kind"] == "unit"]
    product_ids = [node["id"] for node in data["nodes"] if node["kind"] == "product"]

    unit_targets_by_source: dict[str, list[str]] = defaultdict(list)
    for edge in data["edges"]:
        if nodes.get(edge["target"], {}).get("kind") == "unit":
            unit_targets_by_source[edge["source"]].append(edge["target"])

    levels: dict[str, int] = {"feed": 0}
    queue: deque[str] = deque(["feed"])
    while queue:
        source = queue.popleft()
        for target in unit_targets_by_source.get(source, []):
            if target not in levels:
                levels[target] = levels[source] + 1
                queue.append(target)

    for unit_id in unit_ids:
        levels.setdefault(unit_id, 1 + (unit_number(unit_id) % 4))

    max_unit_level = max([levels[unit_id] for unit_id in unit_ids], default=1)
    max_unit_level = max(1, min(max_unit_level, 4))
    x_by_level = {
        level: 0.17 + (0.43 * (level - 1) / max(1, max_unit_level - 1))
        for level in range(1, max_unit_level + 1)
    }

    groups: dict[int, list[str]] = defaultdict(list)
    for unit_id in unit_ids:
        groups[min(levels[unit_id], max_unit_level)].append(unit_id)

    positions: dict[str, tuple[float, float]] = {"feed": (0.055, 0.50)}
    for level in sorted(groups):
        group = sorted(groups[level], key=unit_number)
        for unit_id, y in zip(group, spaced_ys(len(group))):
            positions[unit_id] = (x_by_level[level], y)

    product_y = [0.78, 0.50, 0.22]
    for index, product_id in enumerate(product_ids):
        positions[product_id] = (0.735, product_y[index] if index < len(product_y) else 0.18)

    return positions, nodes


def node_half_size(node: dict | None) -> tuple[float, float]:
    if not node:
        return 0.025, 0.025
    if node["kind"] == "feed":
        return 0.036, 0.036
    if node["kind"] == "product":
        return 0.060, 0.037
    return 0.052, 0.032


def curve_for_edge(
    edge: dict, index: int, positions: dict[str, tuple[float, float]], nodes: dict[str, dict]
) -> tuple[tuple[float, float], tuple[float, float], tuple[float, float], tuple[float, float]]:
    source = positions.get(edge["source"], (0.1, 0.5))
    target = positions.get(edge["target"], (0.7, 0.5))
    source_w, source_h = node_half_size(nodes.get(edge["source"]))
    target_w, target_h = node_half_size(nodes.get(edge["target"]))

    if target[0] >= source[0]:
        start = (source[0] + source_w, source[1])
        end = (target[0] - target_w, target[1])
        dx = max(0.04, end[0] - start[0])
        offset = ((index % 7) - 3) * 0.012
        control1 = (start[0] + dx * 0.52, start[1] + offset)
        control2 = (end[0] - dx * 0.52, end[1] - offset)
        return start, control1, control2, end

    arc_high = index % 2 == 0
    rail_y = 0.93 if arc_high else 0.07
    sign = 1.0 if arc_high else -1.0
    start = (source[0], source[1] + sign * source_h)
    end = (target[0], target[1] + sign * target_h)
    control1 = (source[0] + 0.05, rail_y)
    control2 = (target[0] - 0.05, rail_y)
    return start, control1, control2, end


def bezier(curve: tuple[tuple[float, float], ...], t: float) -> tuple[float, float]:
    p0, p1, p2, p3 = curve
    u = 1.0 - t
    x = (u**3) * p0[0] + 3 * (u**2) * t * p1[0] + 3 * u * (t**2) * p2[0] + (t**3) * p3[0]
    y = (u**3) * p0[1] + 3 * (u**2) * t * p1[1] + 3 * u * (t**2) * p2[1] + (t**3) * p3[1]
    return x, y


def mpl_path(curve: tuple[tuple[float, float], ...]) -> MplPath:
    return MplPath(curve, [MplPath.MOVETO, MplPath.CURVE4, MplPath.CURVE4, MplPath.CURVE4])


def add_box(
    ax,
    center: tuple[float, float],
    size: tuple[float, float],
    face: str,
    edge: str,
    label: str,
    sublabel: str | None = None,
    label_size: int = 8,
) -> None:
    x, y = center
    w, h = size
    patch = FancyBboxPatch(
        (x - w / 2, y - h / 2),
        w,
        h,
        boxstyle="round,pad=0.010,rounding_size=0.014",
        linewidth=1.0,
        edgecolor=edge,
        facecolor=face,
        zorder=5,
    )
    ax.add_patch(patch)
    ax.text(
        x,
        y + (0.006 if sublabel else 0.0),
        label,
        ha="center",
        va="center",
        fontsize=label_size,
        color=INK,
        weight="semibold",
        zorder=6,
    )
    if sublabel:
        ax.text(
            x,
            y - h * 0.26,
            sublabel,
            ha="center",
            va="center",
            fontsize=NODE_SUBLABEL_SIZE,
            color=MUTED,
            zorder=6,
        )


def draw_static_scene(ax, data: dict, positions: dict[str, tuple[float, float]], nodes: dict[str, dict]):
    ax.set_xlim(0, 1)
    ax.set_ylim(0, 1)
    ax.axis("off")
    ax.set_facecolor(BACKGROUND)

    components = data["components"]
    results = data["results"]
    products = results["products"]
    tailings = results["tailings"]
    max_flow = max([edge["total_flow"] for edge in data["edges"]] + [1.0])

    ax.text(0.035, 0.955, "Animated circuit flow", fontsize=20, color=INK, weight="bold", va="top")

    curves = {}
    for index, edge in enumerate(data["edges"]):
        curve = curve_for_edge(edge, index, positions, nodes)
        curves[edge["id"]] = curve
        path = mpl_path(curve)
        width = 0.45 + min(2.4, math.sqrt(max(0.0, edge["total_flow"])) * 0.23)
        alpha = 0.18 + min(0.22, edge["total_flow"] / max_flow * 0.25)
        patch = FancyArrowPatch(
            path=path,
            arrowstyle="-|>",
            mutation_scale=7,
            linewidth=width,
            color=EDGE,
            alpha=alpha,
            zorder=1,
        )
        ax.add_patch(patch)

    feed_total = flow_sum(data["feed_rates"])
    feed = positions["feed"]
    ax.add_patch(Circle(feed, 0.039, facecolor="#daf5e3", edgecolor="#35835b", linewidth=1.1, zorder=5))
    ax.text(feed[0], feed[1] + 0.006, "Feed", ha="center", va="center", fontsize=12,
            color=INK, weight="bold", zorder=6)
    ax.text(feed[0], feed[1] - 0.014, f"{feed_total:.1f} kg/s", ha="center", va="center",
            fontsize=10, color=MUTED, zorder=6)

    for node_id, node in nodes.items():
        if node["kind"] != "unit":
            continue
        colour = "#fff0c2" if node["unit_type"] == "A" else "#dceafe"
        edge_colour = "#a56c00" if node["unit_type"] == "A" else "#3168b2"
        add_box(
            ax,
            positions[node_id],
            (0.092, 0.054),
            colour,
            edge_colour,
            node["label"],
            f"Type {node['unit_type']}",
            label_size=NODE_LABEL_SIZE,
        )

    product_colours = {
        "palusznium_product": ("#ffe49a", "#a57900"),
        "gormanium_product": ("#cfe2ff", "#2b65b1"),
        "tailings_product": ("#e8ebef", "#697386"),
    }
    for node_id, node in nodes.items():
        if node["kind"] != "product":
            continue
        stream = tailings if node["product_index"] == 2 else products[node["product_index"]]
        face, edge = product_colours.get(node_id, ("#f3f4f6", "#64748b"))
        add_box(
            ax,
            positions[node_id],
            (0.112, 0.066),
            face,
            edge,
            node["label"],
            f"{flow_sum(stream):.1f} kg/s",
            label_size=PRODUCT_LABEL_SIZE,
        )

    panel = FancyBboxPatch(
        (PANEL_X, PANEL_Y),
        PANEL_WIDTH,
        PANEL_HEIGHT,
        boxstyle="round,pad=0.014,rounding_size=0.018",
        facecolor="#ffffff",
        edgecolor=PANEL_EDGE,
        linewidth=1.0,
        zorder=4,
    )
    ax.add_patch(panel)
    text_x = PANEL_X + 0.020
    swatch_x = PANEL_X + 0.028
    value_x = PANEL_X + 0.040
    y = PANEL_Y + PANEL_HEIGHT - 0.035
    ax.text(text_x, y, "Results", fontsize=16, weight="bold", color=INK, va="top", zorder=6)
    y -= 0.047
    ax.text(text_x, y, f"Score: {results['score']:.2f} GBP/s", fontsize=11, weight="bold",
            color=INK, va="top", zorder=6)
    y -= 0.034
    ax.text(text_x, y, f"Units: {results['type_a_count']} A, {results['type_b_count']} B",
            fontsize=PANEL_BODY_SIZE, color=MUTED, va="top", zorder=6)
    y -= 0.049
    ax.text(text_x, y, f"Pal recovery {results['pal_recovery']:.1f}% | grade {results['pal_grade']:.1f}%",
            fontsize=PANEL_BODY_SIZE, color=INK, va="top", zorder=6)
    y -= 0.030
    ax.text(text_x, y, f"Gor recovery {results['gor_recovery']:.1f}% | grade {results['gor_grade']:.1f}%",
            fontsize=PANEL_BODY_SIZE, color=INK, va="top", zorder=6)

    def write_stream(title: str, stream: list[float], start_y: float) -> float:
        ax.text(text_x, start_y, title, fontsize=11, weight="bold", color=INK, va="top", zorder=6)
        yy = start_y - 0.034
        for idx, component in enumerate(components):
            ax.add_patch(Circle((swatch_x, yy), 0.0048, facecolor=component["colour"],
                                edgecolor="none", zorder=6))
            ax.text(value_x, yy, f"{component['name']}: {component_at(stream, idx):.2f}",
                    fontsize=PANEL_LIST_SIZE, color=MUTED, va="center", zorder=6)
            yy -= 0.028
        return yy - 0.020

    y -= 0.061
    y = write_stream("Pal product kg/s", products[0], y)
    y = write_stream("Gor product kg/s", products[1], y)
    y = write_stream("Tailings kg/s", tailings, y)

    y = max(y, PANEL_Y + 0.145)
    ax.text(text_x, y, "Legend", fontsize=11, weight="bold", color=INK, va="top", zorder=6)
    legend_y = y - 0.034
    for component in components:
        ax.add_patch(Circle((swatch_x, legend_y), 0.0052, facecolor=component["colour"],
                            edgecolor="none", zorder=6))
        ax.text(value_x, legend_y, component["name"], fontsize=PANEL_LIST_SIZE, color=MUTED,
                va="center", zorder=6)
        legend_y -= 0.028

    return curves


def build_particles(ax, data: dict, curves: dict[str, tuple[tuple[float, float], ...]]):
    components = data["components"]
    max_flow = max(
        [component_at(edge["flows"], idx) for edge in data["edges"] for idx in range(len(components))]
        + [1.0]
    )
    specs = []
    for edge_index, edge in enumerate(data["edges"]):
        curve = curves[edge["id"]]
        for comp_index, component in enumerate(components):
            flow = component_at(edge["flows"], comp_index)
            if flow <= max_flow * 0.003:
                continue
            repeats = 2 if flow > max_flow * 0.18 else 1
            for repeat in range(repeats):
                specs.append((flow, curve, component["colour"], edge_index, comp_index, repeat))

    specs.sort(key=lambda item: item[0], reverse=True)
    specs = specs[:120]

    particles = []
    for rank, (flow, curve, colour, edge_index, comp_index, repeat) in enumerate(specs):
        marker_size = 4.0 + min(4.5, math.sqrt(flow) * 0.45)
        (artist,) = ax.plot(
            [],
            [],
            "o",
            markersize=marker_size,
            markerfacecolor=colour,
            markeredgewidth=0,
            alpha=0.92,
            zorder=3,
        )
        artist.set_path_effects(
            [path_effects.withStroke(linewidth=marker_size * 0.42, foreground=colour, alpha=0.22)]
        )
        speed = 0.78 + min(0.72, math.sqrt(flow / max_flow) * 0.78)
        offset = ((edge_index * 0.173) + (comp_index * 0.117) + (repeat * 0.37) + rank * 0.011) % 1.0
        particles.append((artist, curve, speed, offset))
    return particles


def render(data: dict, output_path: Path) -> None:
    positions, nodes = build_layout(data)
    fig, ax = plt.subplots(figsize=(15.5, 10.8), facecolor=BACKGROUND)
    fig.subplots_adjust(left=0.012, right=0.995, top=0.985, bottom=0.015)
    curves = draw_static_scene(ax, data, positions, nodes)
    particles = build_particles(ax, data, curves)

    frames = 72

    def update(frame: int):
        global_phase = frame / float(frames)
        for artist, curve, speed, offset in particles:
            t = (global_phase * speed + offset) % 1.0
            x, y = bezier(curve, t)
            artist.set_data([x], [y])
        return [artist for artist, *_ in particles]

    animation = FuncAnimation(fig, update, frames=frames, interval=80, blit=False)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    animation.save(str(output_path), writer=PillowWriter(fps=12), dpi=110)
    plt.close(fig)


def main() -> int:
    if len(sys.argv) != 3:
        print("usage: animate_circuit.py payload.json output.gif", file=sys.stderr)
        return 2

    payload_path = Path(sys.argv[1])
    output_path = Path(sys.argv[2])
    with payload_path.open("r", encoding="utf-8") as handle:
        data = json.load(handle)

    render(data, output_path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
