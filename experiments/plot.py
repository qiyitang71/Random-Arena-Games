#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os, glob, re, math
from pathlib import Path
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

# --------------------------- Parsing ---------------------------

def parse_prob_token(token: str) -> float:
    token = token.strip()
    token = re.sub(r'^[A-Za-z_]+\s*[:=]\s*', '', token)  # strip labels like p= or prob:
    if '/' in token:
        a_str, b_str = token.split('/', 1)
        a = float(a_str.strip()); b = float(b_str.strip())
        if b == 0:
            raise ValueError("Zero denominator in fraction for expected probability.")
        return a / b
    if token.endswith('%'):
        return float(token[:-1].strip()) / 100.0
    return float(token)

def parse_line_ab(s: str):
    a_str, b_str = s.strip().split('/', 1)
    return int(a_str), int(b_str)

def read_result_file(path: str) -> pd.DataFrame:
    """
    File format:
      line 1: expected p (e.g., '0.62', 'p=0.62', or '557056/1048576')
      later lines: 'a/b; 3767.768' (RHS after ';' optional)
    Returns: columns [file, n, ratio] for this file.
    """
    with open(path, "r", encoding="utf-8") as f:
        raw = [ln.rstrip("\n") for ln in f]

    lines = [ln for ln in raw if ln.strip() and not ln.lstrip().startswith(('#', '//', ';'))]
    if not lines:
        raise ValueError(f"{path} is empty after removing comments.")

    p_exp = parse_prob_token(re.split(r'[\s,;]+', lines[0].strip())[0])

    rows = []
    for ln in lines[1:]:
        core = ln.split('#', 1)[0].strip()
        if not core:
            continue
        left, _ = (core.split(';', 1) + [""])[:2]
        token = re.split(r'[\s,]+', left.strip())[0]  # first token must be a/b
        wins, n = parse_line_ab(token)
        if n <= 0:
            continue
        p_hat = wins / n
        ratio = p_hat / p_exp
        rows.append((Path(path).stem, n, ratio))

    return pd.DataFrame(rows, columns=["file", "n", "ratio"])

# ---------------------- Aggregation helpers ----------------------

def aggregate_across_files(folder: str, pattern: str = "*.txt"):
    """
    Aggregates absolute ratio error across all files in a folder at each n.
    Returns (agg_dataframe, x_array).
    """
    files = sorted(glob.glob(os.path.join(folder, pattern)))
    if not files:
        raise SystemExit(f"No files matching {pattern!r} in {folder!r}")

    df_all = pd.concat([read_result_file(fp) for fp in files], ignore_index=True)
    df_all["abs_err"] = (df_all["ratio"] - 1.0).abs()

    g = (
        df_all
        .groupby("n", as_index=False)
        .agg(
            mean_abs_err=("abs_err", "mean"),
            std_abs_err=("abs_err", "std"),
            min_abs_err=("abs_err", "min"),
            max_abs_err=("abs_err", "max"),
            count=("abs_err", "count"),
        )
        .sort_values("n")
    )

    g["std_abs_err"] = g["std_abs_err"].fillna(0.0)
    g["se_abs_err"]  = g["std_abs_err"] / np.sqrt(g["count"].clip(lower=1))
    g["lo_abs_err"]  = g["mean_abs_err"] - 1.96 * g["se_abs_err"]
    g["hi_abs_err"]  = g["mean_abs_err"] + 1.96 * g["se_abs_err"]

    x = g["n"].to_numpy(float)
    return g, x

def aggregate_for_folder(folder: str, pattern: str = "*.txt"):
    # alias used by the two-folder plot
    return aggregate_across_files(folder, pattern)

# --------------------------- Hoeffding (YOUR convention) ---------------------------

def hoeffding_line(n_values: np.ndarray, eps: float, delta: float) -> np.ndarray:
    """
    Your form:
      xxxx n >= (ln 2 / (2 eps^2)) * ln(2/delta)
      xxxx => |p_hat - p0| <= sqrt( ln 2 * ln(1/delta) / (2 n) )
      n >= (1 / (2*eps^2)) * ln(2/delta)
      => |p_hat - p0| <= sqrt( ln(2/delta) / (n*2) )
    """
    n_safe = np.maximum(n_values.astype(float), 1.0)
    # return np.sqrt((np.log(2.0) * np.log(1.0 / delta)) / (2.0 * n_safe))
    return np.sqrt((np.log(2.0 / delta)) / (2*n_safe))


def hoeffding_n_eps_delta(eps: float, delta: float) -> int:
    return math.ceil((math.log(2.0) / (2.0 * eps * eps)) * math.log(1.0 / delta))

# --------------------------- Plotting ---------------------------

def plot_per_file(
    folder: str,
    pattern: str = "*.txt",
    eps: float = 0.02,
    delta: float = 0.05,
    show_hoeffding_line: bool = False,
    legend_top: bool = True,
    save_path: str | None = None,
):
    """
    Multi-line figure: one curve per .txt file of |O/E - 1| vs n.
    Legend at top, no title, linear x-axis. Optional Hoeffding dashed LINE.
    """
    files = sorted(glob.glob(os.path.join(folder, pattern)))
    if not files:
        raise SystemExit(f"No files matching {pattern!r} in {folder!r}")

    plt.figure(figsize=(9, 5))

    # plot each benchmark
    all_x = []
    for fp in files:
        df = read_result_file(fp).sort_values("n")
        x = df["n"].to_numpy(float)
        y = np.abs(df["ratio"].to_numpy(float) - 1.0)
        all_x.append(x)
        plt.plot(x, y, marker="o", linewidth=1.5, label=Path(fp).stem)

    # optional Hoeffding LINE
    if show_hoeffding_line and all_x:
        n_min = min(np.min(x) for x in all_x)
        n_max = max(np.max(x) for x in all_x)
        n_grid = np.linspace(max(1.0, float(n_min)), float(n_max), 400)
        h_line = hoeffding_line(n_grid, eps, delta)
        plt.plot(n_grid, h_line, linestyle="--", linewidth=1, label="Hoeffding bound")
        n_hd = hoeffding_n_eps_delta(eps, delta)
        plt.axvline(n_hd, linestyle="--", linewidth=1, label=r"$n_{\varepsilon,\delta}$")

    # guides
    plt.axhline(0.0, linestyle="--", linewidth=1)
    plt.axhline(eps, linestyle=":", linewidth=1, label=f"ε = {eps:.3f}")

    # labels (no title)
    plt.xlabel("Number of samples (n)")
    plt.ylabel("Absolute ratio error  |O/E - 1|")

    # legend at top
    if legend_top:
        ncols = min(len(files) + (2 if show_hoeffding_line else 1), 4)
        plt.legend(loc="lower center", bbox_to_anchor=(0.5, 1.02),
                   ncol=ncols, frameon=True)
    else:
        plt.legend()

    plt.grid(True, linestyle="--", alpha=0.4)
    plt.tight_layout()

    if save_path:
        plt.savefig(save_path, dpi=200, bbox_inches="tight")
    plt.show()

def plot_abs_err_with_ci(
    agg: pd.DataFrame,
    x: np.ndarray,
    eps: float = 0.02,
    delta: float = 0.05,
    show_hoeffding_line: bool = False,
    legend_top: bool = True,
    save_path: str | None = None,
):
    """
    Aggregated figure: mean |O/E - 1| with 95% CI (shaded).
    Optional Hoeffding dashed LINE (not shadow).
    """
    y  = agg["mean_abs_err"].to_numpy(float)
    lo = agg["lo_abs_err"].to_numpy(float)
    hi = agg["hi_abs_err"].to_numpy(float)

    plt.figure(figsize=(9, 5))
    plt.plot(x, y, marker="o", label="Mean |O/E - 1| (across benchmarks)")
    plt.fill_between(x, lo, hi, alpha=0.22, label="95% CI of mean |O/E - 1|")

    if show_hoeffding_line:
        h_line = hoeffding_line(x, eps, delta)
        plt.plot(x, h_line, linestyle="--", linewidth=1, label="Hoeffding bound")
        n_hd = hoeffding_n_eps_delta(eps, delta)
        plt.axvline(n_hd, linestyle="--", linewidth=1, label=r"$n_{\varepsilon,\delta}$")

    plt.axhline(0.0, linestyle="--", linewidth=1)
    plt.axhline(eps, linestyle=":", linewidth=1, label=f"ε = {eps:.3f}")

    plt.xlabel("Number of samples (n)")
    plt.ylabel("Absolute ratio error  |O/E - 1|")
    # no title

    if legend_top:
        plt.legend(loc="lower center", bbox_to_anchor=(0.5, 1.02), ncol=3, frameon=True)
    else:
        plt.legend()

    plt.grid(True, linestyle="--", alpha=0.4)
    plt.tight_layout()

    if save_path:
        plt.savefig(save_path, dpi=200, bbox_inches="tight")
    plt.show()

def plot_abs_err_with_ci_two_folders(
    series: list,              # [(label, agg_df, x_array), ...]
    eps: float = 0.005,
    delta: float = 0.05,
    show_hoeffding_line: bool = True,
    legend_top: bool = True,
    save_path: str | None = None,
):
    """
    Draws one aggregated curve (with its 95% CI) per folder (e.g., 'parity', 'reachability').
    """
    plt.figure(figsize=(9, 5))

    ####################################################
    # Colorblind-safe palette (Okabe & Ito)
    cb_colors = [
        "#0072B2",  # blue
        "#D55E00",  # vermilion
        "#009E73",  # green
        "#CC79A7",  # purple (extra)
        "#E69F00",  # orange (extra)
    ]

    line_styles = ["solid", "dashed", "dotted"]
    markers = ["o", "s", "^"]

    # Only show markers every k points
    marker_stride = 10  # show marker every 5th point
    legend_handles = []  # collect line handles for legend
    var_handle_added = False  # track if variance handle has been added to legend

    for i, (label, agg, x) in enumerate(series):
        y  = agg["mean_abs_err"].to_numpy(float)
        lo = agg["lo_abs_err"].to_numpy(float)
        hi = agg["hi_abs_err"].to_numpy(float)
        min_err = agg["min_abs_err"].to_numpy(float)
        max_err = agg["max_abs_err"].to_numpy(float)

        color = cb_colors[(i) % len(cb_colors)]
        style = line_styles[i % len(line_styles)]
        marker = markers[i % len(markers)]

        # Plot the full line with markers
        # Stagger marker positions: series 0 starts at 0, series 1 at 1, series 2 at 2
        marker_start = (0) % marker_stride
        markevery = (marker_start, marker_stride)  # (start, stride) tuple
        
        (line_handle,) = plt.plot(
            x, y,
            color=color,
            linestyle=style,
            linewidth=1.8,
            marker=marker,
            markersize=4,
            markevery=markevery,  # staggered marker positions
            label=label,
            zorder=3,
        )

        # Variance range shading (min to max across files)
        var_handle = plt.fill_between(
            x, min_err, max_err,
            color=color,
            alpha=0.1,
            zorder=1,
            label="variance range" if not var_handle_added else ""  # only label the first variance
        )
        
        # Add variance handle to legend only once
        if not var_handle_added:
            # legend_handles.append(var_handle)
            var_handle_added = True

        legend_handles.append(line_handle)

    plt.grid(True, linestyle="--", linewidth=0.5, alpha=0.7)
    ####################################################

    # optional shared Hoeffding LINE (over union x-range)
    if show_hoeffding_line and series:
        xmin = min(float(np.min(x)) for _, _, x in series)
        xmax = max(float(np.max(x)) for _, _, x in series)
        n_grid = np.linspace(max(1.0, xmin), xmax, 500)
        h_line = hoeffding_line(n_grid, eps, delta)
        (hoeffding_handle,) = plt.plot(n_grid, h_line, color="gray", linestyle="dashdot", linewidth=1.2, label="Hoeffding bound", zorder=4)
        legend_handles.append(hoeffding_handle)
        n_hd = hoeffding_n_eps_delta(eps, delta)
        # plt.axvline(n_hd, linestyle="--", linewidth=1, label=r"$n_{\varepsilon,\delta}$", zorder=1)

    # guides
    # plt.axhline(0.0, linestyle="--", linewidth=1, zorder=1)
    # plt.axhline(eps, linestyle=":", linewidth=1, label=f"ε = {eps:.3f}", zorder=1)

    # labels (no title)
    plt.xlabel("Number of samples (n)")
    plt.ylabel("Absolute ratio error  |O/E - 1|")

    # force y-axis to start at 0
    plt.ylim(bottom=0)

    if legend_top:
        # Calculate number of columns: series + variance + Hoeffding bound if shown
        ncols = len(series) + 1 + (1 if show_hoeffding_line and series else 0)  # +1 for variance
        plt.legend(
            handles=legend_handles,
            loc="lower center",
            bbox_to_anchor=(0.5, 1.02),
            ncol=ncols,
            frameon=True,
            handlelength=3,
            markerscale=1.8,  # enlarge markers in legend
        )
    else:
        plt.legend(handles=legend_handles, frameon=True, markerscale=1.8)


    plt.grid(True, linestyle="--", alpha=0.4)
    plt.tight_layout()

    if save_path:
        plt.savefig(save_path, dpi=200, bbox_inches="tight")
    plt.show()

# ----------------------------- Run (edit these) -----------------------------

# # Per-file, multi-line view (no aggregation)
# plot_per_file(
#     folder="parity-sample",
#     pattern="*.txt",
#     eps=0.02,
#     delta=0.05,
#     show_hoeffding_line=True,   # dashed line if True
#     legend_top=True,
#     save_path=None               # e.g., "results/per_file_abs_err_line.png"
# )

# plot_per_file(
#     folder="reach-sample",
#     pattern="*.txt",
#     eps=0.02,
#     delta=0.05,
#     show_hoeffding_line=True,   # dashed line if True
#     legend_top=True,
#     save_path=None               # e.g., "results/per_file_abs_err_line.png"
# )

# # Aggregated view with CI (single line + CI + optional Hoeffding line)
# agg, x = aggregate_across_files("parity-sample", "*.txt")
# plot_abs_err_with_ci(
#     agg, x,
#     eps=0.02,
#     delta=0.05,
#     show_hoeffding_line=True,   # dashed line if True
#     legend_top=True,
#     save_path=None               # e.g., "results/agg_abs_err_ci_line.png"
# )

# agg, x = aggregate_across_files("reach-sample", "*.txt")
# plot_abs_err_with_ci(
#     agg, x,
#     eps=0.02,
#     delta=0.05,
#     show_hoeffding_line=True,   # dashed line if True
#     legend_top=True,
#     save_path=None               # e.g., "results/agg_abs_err_ci_line.png"
# )

# Aggregated view with CI for TWO folders (e.g., parity vs reachability)
agg_parity,  x_parity  = aggregate_for_folder("results/parity-sample", "*.txt")
agg_reach,   x_reach   = aggregate_for_folder("results/reach-sample", "*.txt")
agg_energy,  x_energy   = aggregate_for_folder("results/energy-sample", "*.txt")

plot_abs_err_with_ci_two_folders(
    series=[
        ("reachability", agg_reach, x_reach),
        ("parity", agg_parity, x_parity),
        ("energy", agg_energy, x_energy),

    ],
    eps=0.02,
    delta=0.05,
    show_hoeffding_line=True,    # dashed line (shared bound), set False to hide
    legend_top=True,
    save_path="plot.png"
)
