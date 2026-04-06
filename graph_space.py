#!/usr/bin/env python3
"""Build, run the Big O space demo, and generate Plotly charts for binary trees."""

import csv
import subprocess
import sys
import webbrowser
from pathlib import Path

try:
    import plotly.graph_objects as go
    from plotly.subplots import make_subplots
except ImportError:
    print("plotly not found -- installing...")
    subprocess.check_call([sys.executable, "-m", "pip", "install", "plotly"])
    import plotly.graph_objects as go
    from plotly.subplots import make_subplots

try:
    import kaleido  # noqa: F401 -- just check it's available
except ImportError:
    print("kaleido not found -- installing (needed for SVG export)...")
    subprocess.check_call([sys.executable, "-m", "pip", "install", "kaleido"])


import math

REPO_DIR    = Path(__file__).resolve().parent
CSV_FILE    = REPO_DIR / "results_space.csv"
OUTPUT_HTML = REPO_DIR / "charts_space.html"
IMAGES_DIR  = REPO_DIR / "images"
IMAGES_DIR.mkdir(exist_ok=True)


# -- Build & Run --------------------------------------------------------------

def find_executable():
    out_build = REPO_DIR / "out" / "build"
    if out_build.exists():
        for exe in out_build.rglob("big-o-demo-binary-trees-space.exe"):
            if "CompilerId" not in str(exe):
                return exe
    candidates = [
        REPO_DIR / "build" / "big-o-demo-binary-trees-space.exe",
        REPO_DIR / "build" / "Debug"   / "big-o-demo-binary-trees-space.exe",
        REPO_DIR / "build" / "Release" / "big-o-demo-binary-trees-space.exe",
        REPO_DIR / "build" / "big-o-demo-binary-trees-space",
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


# -- Parse CSV ----------------------------------------------------------------

def read_results():
    rows = []
    with open(CSV_FILE) as f:
        for row in csv.DictReader(f):
            rows.append({
                "scenario":   row["scenario"],
                "complexity": row["complexity"],
                "n":          int(row["n"]),
                "heap_bytes": int(row["heap_bytes"]),
                "height":     int(row["height"]),
            })
    return rows


def group_by_scenario(rows):
    series = {}
    for row in rows:
        key = row["scenario"]
        if key not in series:
            series[key] = {"n": [], "heap": [], "height": [], "complexity": row["complexity"]}
        series[key]["n"].append(row["n"])
        series[key]["heap"].append(row["heap_bytes"])
        series[key]["height"].append(row["height"])
    return series


# -- Chart colours / styles ---------------------------------------------------

COLORS = {
    "BST (random)": "#16a34a",   # green
    "BST (sorted)": "#dc2626",   # red
    "AVL (sorted)": "#2563eb",   # blue
}

LABELS = {
    "BST (random)": "BST (random data) — O(log n) height",
    "BST (sorted)": "BST (sorted data) — O(n) height  ← DEGENERATE",
    "AVL (sorted)": "AVL (sorted data) — O(log n) height",
}


# -- Chart 1: Heap memory -----------------------------------------------------

def build_heap_chart(series):
    fig = go.Figure()

    order = ["BST (random)", "BST (sorted)", "AVL (sorted)"]
    for key in order:
        if key not in series:
            continue
        data = series[key]
        # Add reference line: O(n) * bytes_per_node (approximate slope from first point)
        fig.add_trace(go.Scatter(
            x=data["n"],
            y=data["heap"],
            mode="lines+markers",
            name=LABELS[key],
            line=dict(color=COLORS[key], width=3),
            marker=dict(size=10),
        ))

    fig.update_layout(
        title=dict(
            text="Heap Memory: Node Allocation vs Input Size",
            font=dict(size=20),
            subtitle=dict(
                text="All three scenarios allocate exactly n nodes → O(n) heap. "
                     "AVL nodes are 32 B vs 24 B for BST (stores cached height).",
                font=dict(size=13, color="#666"),
            ),
        ),
        xaxis_title="n (number of nodes inserted)",
        yaxis_title="Heap bytes allocated for nodes",
        yaxis=dict(rangemode="tozero"),
        template="plotly_white",
        font=dict(size=14),
        legend=dict(font=dict(size=13)),
        margin=dict(t=80, b=60),
        height=500,
    )
    return fig


# -- Chart 2: Tree height (recursion depth) -----------------------------------

def build_height_chart(series):
    fig = go.Figure()

    # Add theoretical O(log n) reference
    all_n = sorted({n for data in series.values() for n in data["n"]})
    ref_n = list(range(min(all_n), max(all_n) + 1, max(all_n) // 200))
    log_ref = [1.5 * math.log2(n) for n in ref_n]
    fig.add_trace(go.Scatter(
        x=ref_n,
        y=log_ref,
        mode="lines",
        name="1.5 × log₂(n) reference",
        line=dict(color="#94a3b8", width=2, dash="dot"),
        opacity=0.7,
    ))

    order = ["BST (random)", "BST (sorted)", "AVL (sorted)"]
    for key in order:
        if key not in series:
            continue
        data = series[key]
        fig.add_trace(go.Scatter(
            x=data["n"],
            y=data["height"],
            mode="lines+markers",
            name=LABELS[key],
            line=dict(color=COLORS[key], width=3),
            marker=dict(size=10),
        ))

    fig.update_layout(
        title=dict(
            text="Tree Height = Maximum Recursion Depth per Operation",
            font=dict(size=20),
            subtitle=dict(
                text="Height determines how many stack frames any insert/search/remove call needs. "
                     "Each frame ≈ 64 bytes on x64.",
                font=dict(size=13, color="#666"),
            ),
        ),
        xaxis_title="n (number of nodes inserted)",
        yaxis_title="Tree height (= max recursion depth)",
        yaxis=dict(rangemode="tozero"),
        template="plotly_white",
        font=dict(size=14),
        legend=dict(font=dict(size=13)),
        margin=dict(t=80, b=60),
        height=500,
    )
    return fig


# -- Generate HTML ------------------------------------------------------------

def generate_html(series):
    heap_fig   = build_heap_chart(series)
    height_fig = build_height_chart(series)

    # Export static SVGs for embedding in Canvas concept page
    heap_svg   = IMAGES_DIR / "space_heap_chart.svg"
    height_svg = IMAGES_DIR / "space_height_chart.svg"
    heap_fig.write_image(str(heap_svg),   width=900, height=500)
    height_fig.write_image(str(height_svg), width=900, height=500)
    print(f"SVGs written to {IMAGES_DIR.name}/")


    heap_div   = heap_fig.to_html(full_html=False, include_plotlyjs=True)
    height_div = height_fig.to_html(full_html=False, include_plotlyjs=False)

    # Pull summary values at the largest shared n
    largest_n = max(n for data in series.values() for n in data["n"])
    summary = {}
    for key, data in series.items():
        if largest_n in data["n"]:
            idx = data["n"].index(largest_n)
            summary[key] = {
                "heap": data["heap"][idx],
                "height": data["height"][idx],
                "stack_kb": data["height"][idx] * 64 // 1024,
            }

    def fmt(b):
        if b >= 1_000_000: return f"{b/1_000_000:.1f} MB"
        if b >= 1_000:     return f"{b/1_000:.1f} KB"
        return f"{b} B"

    html = f"""<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<title>Big O Demo: Binary Trees -- Space Complexity</title>
<style>
  body {{ background: #ffffff; color: #1a1a1a; font-family: system-ui, sans-serif;
         max-width: 960px; margin: 0 auto; padding: 2rem; }}
  h1 {{ text-align: center; margin-bottom: 0.25rem; }}
  p.sub {{ text-align: center; color: #666; margin-top: 0; margin-bottom: 1.5rem; }}
  .chart {{ margin-bottom: 1.5rem; }}
  .panel {{ border-radius: 10px; padding: 1.25rem 1.5rem; margin-bottom: 1.25rem;
            font-size: .9rem; line-height: 1.6; }}
  .panel h3 {{ margin: 0 0 .6rem; font-size: 1rem; }}
  .panel ul {{ padding-left: 1.3rem; margin: .4rem 0; }}
  .panel li {{ margin-bottom: .35rem; }}
  .panel code {{ background: #e8e8e8; padding: .1em .35em; border-radius: 4px;
                 font-family: Consolas, monospace; font-size: .88em; }}
  table {{ border-collapse: collapse; width: 100%; font-size: .9rem; margin: .75rem 0; }}
  th {{ background: #f1f5f9; padding: .5rem .85rem; text-align: left; }}
  td {{ padding: .45rem .85rem; border-bottom: 1px solid #e2e8f0; }}
</style>
</head>
<body>
<h1>Big O Demo: Binary Trees &mdash; Space Complexity</h1>
<p class="sub">Two metrics: heap memory for nodes (always O(n)) and tree height (= max recursion depth)</p>

<!-- CHART 1: HEAP -->
<div class="chart">{heap_div}</div>

<!-- WHY ALL THREE ARE THE SAME -->
<div class="panel" style="background: #e8f5e9; border-left: 5px solid #1b5e20;">
  <h3 style="color: #1b5e20;">Why are all three scenarios roughly the same heap usage?</h3>
  <p>Every tree stores exactly <strong>n nodes</strong>, regardless of its shape. A BST and an AVL
  tree with 10,000 values both allocate 10,000 node objects on the heap &mdash; the shape
  (balanced vs degenerate) does not change the node count.</p>
  <ul>
    <li><strong>BST node</strong>: <code>int data</code> + <code>Node* left</code> + <code>Node* right</code>
    = 4 + 8 + 8 = <strong>20 bytes</strong> (24 with alignment padding)</li>
    <li><strong>AVL node</strong>: same as BST + <code>int height</code> = 4 + 8 + 8 + 4
    = <strong>24 bytes</strong> (32 with alignment padding)</li>
    <li>The slight gap between the blue (AVL) and green/red (BST) lines is just those 8 extra bytes
    per node for the cached height field</li>
  </ul>
  <p><strong>Bottom line:</strong> for tree <em>node</em> storage, all structures are O(n).
  The space trade-off is elsewhere &mdash; in the <em>call stack</em>.</p>
</div>

<!-- CHART 2: HEIGHT -->
<div class="chart">{height_div}</div>

<!-- STACK DEPTH STORY -->
<div class="panel" style="background: #ffebee; border-left: 5px solid #c62828;">
  <h3 style="color: #c62828;">The real space cost: recursion depth and stack overflow risk</h3>
  <p>Every BST operation (insert, search, remove) is recursive. Each recursive call adds one
  <strong>stack frame</strong> (≈&nbsp;64&nbsp;bytes on x64). The tree height tells you exactly
  how many frames deep any single operation can go:</p>
  <table>
    <tr>
      <th>Scenario</th>
      <th>Height at n={largest_n:,}</th>
      <th>Heap (nodes)</th>
      <th>Stack depth estimate</th>
    </tr>
    {"".join(f"<tr><td><strong>{k}</strong></td><td>{summary[k]['height']:,}</td><td>{fmt(summary[k]['heap'])}</td><td>~{summary[k]['stack_kb']:,} KB</td></tr>" for k in ["BST (random)", "BST (sorted)", "AVL (sorted)"] if k in summary)}
  </table>
  <ul>
    <li><strong>BST (random):</strong> height ≈ 1.4&thinsp;log₂(n) &mdash;
    the tree stays roughly balanced by luck. A search on a 10,000-node tree goes
    about 14 levels deep &mdash; completely safe.</li>
    <li><strong>BST (sorted):</strong> height = n&minus;1 &mdash; inserting 1, 2, 3&hellip;
    creates a right spine: every node is a right child and has no left child.
    It looks and behaves exactly like a linked list. A search for the last element
    visits all 10,000 nodes and needs 10,000 stack frames ≈ 625&nbsp;KB of stack.
    Default stack size on Windows is 1&nbsp;MB, so large n values will crash.</li>
    <li><strong>AVL (sorted):</strong> rotations keep height ≤ 1.44&thinsp;log₂(n) even with
    sorted input. Same as the random BST case &mdash; O(log n) stack depth, always safe.</li>
  </ul>
</div>

<!-- TAKEAWAY -->
<div class="panel" style="background: #fff8e1; border-left: 5px solid #f9a825;">
  <h3 style="color: #c79100;">Key Takeaway: Same O(n) nodes, wildly different stack depth</h3>
  <p>The BST and AVL tree with n nodes use the same amount of heap memory. But the
  <em>shape</em> of the tree determines how deep every recursive operation goes, which
  is what determines whether your program will <strong>crash</strong> or not on large inputs.</p>
  <ul>
    <li>O(log n) height (balanced BST or AVL): stack depth is tiny &mdash; about 14 frames at n=10,000</li>
    <li>O(n) height (degenerate BST): stack depth equals n &mdash; crashes around n=20,000&ndash;50,000</li>
    <li>AVL trees pay 8 extra bytes per node (the cached height) to <em>guarantee</em> O(log n)
    height regardless of insertion order</li>
  </ul>
  <p>This is why balance matters: not just for speed (O(log n) vs O(n) time), but for
  <strong>safety</strong> (stack overflow).</p>
</div>

</body>
</html>"""

    OUTPUT_HTML.write_text(html, encoding="utf-8")
    print(f"\nCharts written to {OUTPUT_HTML}")
    webbrowser.open(OUTPUT_HTML.as_uri())


# -- Main ---------------------------------------------------------------------

if __name__ == "__main__":
    if "--graph-only" not in sys.argv:
        build_and_run()
    else:
        if not CSV_FILE.exists():
            sys.exit(f"ERROR: {CSV_FILE} not found -- run without --graph-only first")
        print("Skipping build/run -- using existing results_space.csv")
    rows   = read_results()
    series = group_by_scenario(rows)
    generate_html(series)
