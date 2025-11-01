# Parameter Selection Visualization Guide

This document explains all the visualizations generated for parameter tuning analysis.

## Generated Visualizations

For each algorithm-dataset combination (LSH-MNIST, LSH-SIFT, Hypercube-MNIST, Hypercube-SIFT), the following visualizations are created:

### 1. Summary Table (`*_summary_table.png`)
**Purpose:** Quick comparison of key configurations
- **Best Recall:** Configuration achieving highest recall
- **Fastest (QPS):** Configuration with highest queries per second
- **Best Speedup:** Configuration with best tTrue/tApprox ratio
- **Balanced (Pareto):** Recommended configuration balancing recall and speed

**How to use:** This table directly justifies your final parameter choice by showing it's on the Pareto frontier with optimal balance.

### 2. Top-20 Configurations Table (`*_sorted_by_recall_top20.png`)
**Purpose:** Detailed view of best-performing configurations
- Sorted by recall (descending)
- Shows all parameter values and metrics
- Top 20 candidates for final selection

### 3. Individual Parameter Plots (`*_recall_vs_<param>.png`)
**Purpose:** Show how each parameter affects recall
- X-axis: Parameter value (k, L, w, kproj, M, probes)
- Y-axis: Recall achieved
- Color: Query time (yellow=fast, red=slow)

**Interpretation:**
- Identify optimal parameter ranges
- See speed/accuracy trade-offs for each parameter
- Find "sweet spots" where recall increases without major slowdown

### 4. Recall vs Time with Pareto Frontier (`*_recall_vs_tapprox_pareto.png`)
**Purpose:** Show fundamental speed/accuracy trade-off
- X-axis: Approximate query time (seconds)
- Y-axis: Recall
- Blue line: Pareto frontier (no config dominates these)
- Blue circles: Pareto-optimal configurations

**How to justify selection:**
- Points on the Pareto frontier are optimal (can't improve one metric without hurting the other)
- Your chosen config should be on or near this frontier
- Shows why you didn't choose faster (lower recall) or more accurate (too slow) configs

### 5. Recall vs QPS with Pareto Frontier (`*_recall_vs_qps_pareto.png`)
**Purpose:** Speed/accuracy trade-off in more intuitive units
- X-axis: Queries Per Second (higher = better)
- Y-axis: Recall (higher = better)
- Pareto frontier shows best possible configurations
- Upper-right corner is ideal (high recall + high throughput)

**Use case:** Better for presentations than time-based plots (QPS more intuitive)

### 6. Recall vs Speedup with Pareto Frontier (`*_recall_vs_speedup_pareto.png`)
**Purpose:** Show how much faster than brute-force at each recall level
- X-axis: Speedup factor (tTrue / tApprox)
- Y-axis: Recall
- Shows efficiency gains clearly (e.g., "10x faster than exact search at 80% recall")

**How to justify selection:**
- Emphasizes practical speedup achieved
- Shows diminishing returns (e.g., 90% recall may only be 5x faster, but 70% recall is 20x faster)
- Makes the speed/accuracy trade-off concrete

### 7. Parameter Pair Heatmaps (`*_heatmap_<param1>_vs_<param2>.png`)
**Purpose:** Visualize interactions between parameters
- X-axis: First parameter
- Y-axis: Second parameter
- Color: Recall achieved
- Numbers: Exact recall values

**Examples:**
- **LSH:** `k vs L`, `k vs w`, `L vs w` heatmaps
- **Hypercube:** `kproj vs w`, `M vs probes`, etc.

**Interpretation:**
- Dark red = high recall
- White/yellow = low recall
- Reveals parameter interactions (e.g., "large k needs large L for good recall")
- Helps identify parameter combinations to avoid

## Colormap Legend

All time-based coloring uses yellow-to-red gradient:
- **Bright Yellow (#FFFF00):** Very fast queries (near 0s)
- **Orange (#FFA500):** Moderate speed
- **Orange-Red (#FF4500):** Getting slower
- **Crimson (#DC143C):** Approaching brute-force time
- **Dark Red (#8B0000):** As slow as or slower than brute-force

Color scale normalized to `max(tTrue)` so red = brute-force baseline.

## Using These for Parameter Selection

### Step 1: Check Summary Table
Look at the "Balanced (Pareto)" row - this is the recommended configuration.

### Step 2: Verify with Pareto Plots
Confirm your chosen config is on the Pareto frontier in:
- Recall vs QPS plot
- Recall vs Speedup plot

### Step 3: Check Parameter Heatmaps
Verify no obviously better parameter combinations were missed.

### Step 4: Review Individual Parameter Plots
Understand how each parameter contributes to final performance.

### For Your Report/Presentation

**Recommended figures to include:**

1. **Summary table** - Shows you systematically evaluated multiple criteria
2. **Recall vs QPS Pareto plot** - Clearly shows your choice is optimal
3. **2-3 key heatmaps** - Demonstrates thorough parameter space exploration
4. **Recall vs Speedup plot** - Quantifies practical speedup achieved

**Narrative example:**
> "We performed a grid search over [parameters]. The Pareto frontier analysis (Fig. X) identified 8 optimal configurations. Among these, we selected the configuration with parameters [values] as it achieves [recall]% recall at [QPS] queries/second, providing a [speedup]x speedup over exact search while maintaining practical accuracy. This represents the best balance point as shown in the summary table (Fig. Y), where alternative configurations either sacrifice too much recall ([config A]) or provide insufficient speedup ([config B])."

## Files Location

All visualizations are saved in:
- `runs/hypercube_mnist/` - Hypercube on MNIST
- `runs/hypercube_sift_100/` - Hypercube on SIFT-100
- `runs/lsh_mnist_100/` - LSH on MNIST-100  
- `runs/lsh_sift_100/` - LSH on SIFT-100

## Regenerating Plots

To regenerate all visualizations:
```bash
python3 Python_Scripts/plot_csv_metrics.py
```

To plot specific CSV files:
```bash
python3 Python_Scripts/plot_csv_metrics.py path/to/your/results.csv
```
