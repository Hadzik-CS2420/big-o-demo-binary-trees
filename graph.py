#!/usr/bin/env python3
"""Build, run the Big O demo, and generate Plotly charts for binary trees."""

import csv
import subprocess
import sys
import webbrowser
from pathlib import Path

try:
    import plotly.graph_objects as go
except ImportError:
    print("plotly not found -- installing...")
    subprocess.check_call([sys.executable, "-m", "pip", "install", "plotly"])
    import plotly.graph_objects as go

REPO_DIR = Path(__file__).resolve().parent
CSV_FILE = REPO_DIR / "results.csv"
OUTPUT_HTML = REPO_DIR / "charts.html"


# -- Build & Run --------------------------------------------------------------

def find_executable():
    out_build = REPO_DIR / "out" / "build"
    if out_build.exists():
        for exe in out_build.rglob("big-o-demo-binary-trees.exe"):
            if "CompilerId" not in str(exe):
                return exe
    candidates = [
        REPO_DIR / "build" / "big-o-demo-binary-trees.exe",
        REPO_DIR / "build" / "Debug" / "big-o-demo-binary-trees.exe",
        REPO_DIR / "build" / "Release" / "big-o-demo-binary-trees.exe",
        REPO_DIR / "build" / "big-o-demo-binary-trees",
    ]
    for path in candidates:
        if path.exists():
            return path
    return None


def build_and_run():
    exe = find_executable()
    if exe is None:
        print("No executable found -- building with CMake...")
        build_dir = REPO_DIR / "build"
        subprocess.run(["cmake", "-B", str(build_dir), str(REPO_DIR)], check=True)
        subprocess.run(["cmake", "--build", str(build_dir)], check=True)
        exe = find_executable()
        if exe is None:
            sys.exit("ERROR: build succeeded but executable not found")
    print(f"Running {exe.name}...\n")
    subprocess.run([str(exe)], cwd=str(REPO_DIR), check=True)


# -- Parse CSV -----------------------------------------------------------------

def read_results():
    with open(CSV_FILE) as f:
        return list(csv.DictReader(f))


def group_by_series(rows):
    """Group by (operation, structure) to get one line per series."""
    series = {}
    for row in rows:
        key = f"{row['structure']} — {row['operation']}"
        if key not in series:
            series[key] = {"n": [], "time": [], "complexity": row["complexity"]}
        series[key]["n"].append(int(row["n"]))
        series[key]["time"].append(float(row["time_us"]))
    return series


# -- Chart Generation ----------------------------------------------------------

COLORS = {
    "BST (random) — insert":  "#16a34a",  # green
    "BST (random) — search":  "#15803d",  # dark green
    "BST (sorted) — insert":  "#dc2626",  # red
    "BST (sorted) — search":  "#b91c1c",  # dark red
    "AVL (sorted) — insert":  "#2563eb",  # blue
    "AVL (sorted) — search":  "#1d4ed8",  # dark blue
}

DASHES = {
    "insert": "solid",
    "search": "dash",
}

LEGEND_GROUPS = [
    {
        "group": "balanced",
        "title": "O(log n) — BST (random data)",
        "series": ["BST (random) — insert", "BST (random) — search"],
    },
    {
        "group": "degenerate",
        "title": "O(n) — BST (sorted → degenerate)",
        "series": ["BST (sorted) — insert", "BST (sorted) — search"],
    },
    {
        "group": "avl",
        "title": "O(log n) — AVL (sorted, rotations fix it)",
        "series": ["AVL (sorted) — insert", "AVL (sorted) — search"],
    },
]


def build_chart(all_series):
    fig = go.Figure()

    for grp in LEGEND_GROUPS:
        for key in grp["series"]:
            if key not in all_series:
                continue
            data = all_series[key]
            op = key.split(" — ")[1]  # "insert" or "search"
            fig.add_trace(go.Scatter(
                x=data["n"],
                y=data["time"],
                mode="lines+markers",
                name=key,
                legendgroup=grp["group"],
                legendgrouptitle=dict(
                    text=grp["title"], font=dict(size=13, color="#555"),
                ),
                line=dict(
                    color=COLORS.get(key, "#3b82f6"),
                    width=3,
                    dash=DASHES.get(op, "solid"),
                ),
                marker=dict(size=10),
            ))

    fig.update_layout(
        title=dict(
            text="Binary Tree Operations: O(log n) vs O(n)",
            font=dict(size=22),
        ),
        xaxis_title="n (input size)",
        yaxis_title="Time per operation (µs)",
        yaxis=dict(rangemode="tozero"),
        template="plotly_white",
        font=dict(size=14),
        legend=dict(
            font=dict(size=13),
            groupclick="togglegroup",
            tracegroupgap=12,
        ),
        margin=dict(t=60, b=60),
        height=600,
    )
    return fig


def generate_html(all_series):
    fig = build_chart(all_series)
    chart_div = fig.to_html(full_html=False, include_plotlyjs=True)

    html = f"""<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<title>Big O Demo: Binary Trees & AVL</title>
<style>
  body {{ background: #ffffff; color: #1a1a1a; font-family: system-ui, sans-serif;
         max-width: 960px; margin: 0 auto; padding: 2rem; }}
  h1 {{ text-align: center; margin-bottom: 0.25rem; }}
  p.sub {{ text-align: center; color: #666; margin-top: 0; }}
  .chart {{ margin-bottom: 2rem; }}
</style>
</head>
<body>
<h1>Big O Demo: Binary Trees & AVL</h1>
<p class="sub">O(log n) balanced vs O(n) degenerate — per-operation timing</p>
<div class="chart">{chart_div}</div>
</body>
</html>"""

    OUTPUT_HTML.write_text(html, encoding="utf-8")
    print(f"\nCharts written to {OUTPUT_HTML}")
    webbrowser.open(OUTPUT_HTML.as_uri())


# -- Main ----------------------------------------------------------------------

if __name__ == "__main__":
    if "--graph-only" not in sys.argv:
        build_and_run()
    else:
        if not CSV_FILE.exists():
            sys.exit(f"ERROR: {CSV_FILE} not found -- run without --graph-only first")
        print("Skipping build/run -- using existing results.csv")
    rows = read_results()
    all_series = group_by_series(rows)
    generate_html(all_series)
