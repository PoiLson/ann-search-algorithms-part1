# Quick Reference: Parameter Selection Visualizations

## 📋 What You Have Now

### ✨ NEW Enhanced Visualizations (Just Created!)

#### 1. Pareto Frontier Analysis
- Shows configurations where you can't improve recall without sacrificing speed
- Blue dashed line connects optimal points
- **Files:** `*_recall_vs_qps_pareto.png`, `*_recall_vs_speedup_pareto.png`, `*_recall_vs_tapprox_pareto.png`

#### 2. QPS-Based Plots  
- More intuitive than time (higher = better on both axes)
- **File:** `*_recall_vs_qps_pareto.png` ⭐ USE THIS for presentations

#### 3. Speedup Analysis
- Shows X× faster than brute-force at each recall level
- **File:** `*_recall_vs_speedup_pareto.png`

#### 4. Parameter Heatmaps
- 2D color grids showing recall for parameter pairs
- Reveals parameter interactions clearly
- **Files:** `*_heatmap_<param1>_vs_<param2>.png` (one per pair)

#### 5. Summary Tables
- Compares: Best Recall, Fastest, Best Speedup, **Balanced (Recommended)**
- **File:** `*_summary_table.png` ⭐ START HERE

#### 6. Algorithm Comparisons
- LSH vs Hypercube side-by-side
- **Files:** `runs/comparison_mnist.png`, `runs/comparison_sift.png`

## 🎯 Quick Start (3 Steps)

### Step 1: Check Summary Table
```bash
# Open this first:
runs/lsh_mnist_100_grid_summary_table.png
# Look at "Balanced (Pareto)" row - that's your recommendation
```

### Step 2: Verify on Pareto Plot
```bash
# Confirm it's on the frontier:
runs/lsh_mnist_100_grid_recall_vs_qps_pareto.png
```

### Step 3: Annotate Your Choice
```bash
# Highlight your selected config (find row index in CSV):
python3 Python_Scripts/annotate_final_choice.py runs/lsh_mnist_100_grid.csv 42
# Creates: runs/lsh_mnist_100_grid_FINAL_CHOICE_annotated.png
```

## 📊 For Your Report/Presentation

### Minimum figures to include:
1. ⭐ **Summary table** - Shows systematic evaluation
2. ⭐ **Annotated QPS/Pareto plot** - Marks your choice
3. ⭐ **1-2 heatmaps** - Shows parameter exploration

### Advanced (if space allows):
4. **Algorithm comparison** - LSH vs Hypercube
5. **Speedup plot** - Quantifies efficiency gains
6. **Top-20 table** - Detailed candidate configs

## 🔧 Commands

### Regenerate All Plots
```bash
python3 Python_Scripts/plot_csv_metrics.py
```

### Annotate Final Choice
```bash
python3 Python_Scripts/annotate_final_choice.py <csv_path> <row_index>
# Example:
python3 Python_Scripts/annotate_final_choice.py runs/lsh_mnist_100_grid.csv 42
```

### Algorithm Comparison
```bash
python3 Python_Scripts/compare_algorithms.py
```

### Plot Specific CSV
```bash
python3 Python_Scripts/plot_csv_metrics.py path/to/your/custom.csv
```

## 📂 Where Are My Plots?

```
runs/
├── comparison_mnist.png              # LSH vs Hypercube on MNIST
├── comparison_sift.png               # LSH vs Hypercube on SIFT
├── hypercube_mnist_custom_grid/
│   ├── *_summary_table.png           ⭐ START HERE
│   ├── *_recall_vs_qps_pareto.png    ⭐ KEY PLOT
│   ├── *_recall_vs_speedup_pareto.png
│   ├── *_heatmap_*.png               (one per parameter pair)
│   └── ... (per-parameter plots)
├── hypercube_sift_100/               (same structure)
├── lsh_mnist_100/                    (same structure)
└── lsh_sift_100/                     (same structure)
```

## 💬 Justification Template (Copy-Paste!)

```
We performed a grid search over [N] configurations for [algorithm] on [dataset].
Pareto frontier analysis identified [X] optimal configurations (see Fig. Y).

Selected configuration:
- Parameters: [list them]
- Recall: [value]%
- QPS: [value] queries/second  
- Speedup: [X]× faster than brute-force

This configuration achieves the best balance between accuracy and speed among 
Pareto-optimal points. Higher-recall alternatives ([config A]) are [Y]× slower,
while faster alternatives ([config B]) sacrifice [Z]% recall.
```

## 🎨 Color Legend

**Yellow → Red Gradient:**
- 🟡 Yellow = Fast (good!)
- 🟠 Orange = Medium speed
- 🔴 Red = Slow (near brute-force)

## 📚 Full Documentation

- `COMPLETE_SUMMARY.md` - Comprehensive guide
- `VISUALIZATION_GUIDE.md` - Detailed plot explanations

## ✅ What Makes These Plots Good for Parameter Selection?

1. ✅ **Pareto frontier** - Shows you chose from truly optimal configs
2. ✅ **Multiple metrics** - Recall, QPS, speedup all visualized
3. ✅ **Parameter interactions** - Heatmaps reveal non-obvious patterns
4. ✅ **Clear recommendation** - "Balanced" config automatically identified
5. ✅ **Annotation support** - Can mark your exact choice
6. ✅ **Professional quality** - 200-300 DPI, publication-ready
7. ✅ **Comparative analysis** - Can compare algorithms side-by-side

Your parameter selection is now bulletproof! 🛡️
