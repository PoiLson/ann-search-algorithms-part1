#!/usr/bin/env bash
set -euo pipefail

# Hypercube Parameter Tuning Script (SIFT, 100 queries)
# Grid: kproj={16,17,18,19}, w={5,10,20,50,80}, M={5000,8500}, probes={500,850}

OUT_DIR="runs/hypercube_sift_100"
mkdir -p "$OUT_DIR"
CSV="$OUT_DIR/hypercube_sift_100_grid.csv"
: > "$CSV"
echo "KPROJ,W,M,PROBES,AvgRecall,AvgAF,AvgTApprox,AvgTTrue,QPS" >> "$CSV"

# Parameter grids
KPROJ_LIST=(16 17 18 19)
W_LIST=(5 10 20 50 80)
M_LIST=(5000 8500 10000 20000)
PROBES_LIST=(500 850 1000)

# Datasets
DATASET="Data/SIFT/sift_base.fvecs"
QUERIES="Data/SIFT/sift_query_100.fvecs"

for kproj in "${KPROJ_LIST[@]}"; do
  for w in "${W_LIST[@]}"; do
    for m in "${M_LIST[@]}"; do
      for probes in "${PROBES_LIST[@]}"; do
        echo "Running Hypercube (SIFT-100): kproj=$kproj w=$w m=$m probes=$probes" >&2
        tmpf=$(mktemp)
        if ! ./search -d "$DATASET" -q "$QUERIES" -kproj "$kproj" -w "$w" -M "$m" -probes "$probes" -o "$tmpf" -N 1 -R 50000 -type sift -range false -hypercube >/dev/null 2>&1; then
          echo "search returned non-zero for kproj=$kproj w=$w m=$m probes=$probes" >&2
        fi

    if [[ -f "$tmpf" ]]; then
  recall=$(grep -F "Recall@N:" "$tmpf" | tail -1 | sed -E 's/.*: *([0-9.+-eE]+|inf|nan).*/\1/' || echo 0)
  af=$(grep -F "Average AF:" "$tmpf" | tail -1 | sed -E 's/.*: *([0-9.+-eE]+|inf|nan).*/\1/' || echo nan)
  tapprox=$(grep -F "tApproximateAverage:" "$tmpf" | tail -1 | sed -E 's/.*: *([0-9.+-eE]+|inf|nan).*/\1/' || echo 0)
  ttrue=$(grep -F "tTrueAverage:" "$tmpf" | tail -1 | sed -E 's/.*: *([0-9.+-eE]+|inf|nan).*/\1/' || echo 0)
  qps=$(grep -F "QPS:" "$tmpf" | tail -1 | sed -E 's/.*: *([0-9.+-eE]+|inf|nan).*/\1/' || echo 0)
          recall=${recall:-0}; af=${af:-nan}; tapprox=${tapprox:-0}; ttrue=${ttrue:-0}; qps=${qps:-0}
          echo "$kproj,$w,$m,$probes,$recall,$af,$tapprox,$ttrue,$qps" >> "$CSV"
          rm -f "$tmpf"
          echo "Appended kproj=$kproj,w=$w,m=$m,probes=$probes -> recall=${recall} af=${af} tApprox=${tapprox} tTrue=${ttrue} qps=${qps}" >&2
        else
          echo "$kproj,$w,$m,$probes,0,nan,0,0,0" >> "$CSV"
          echo "Missing output for kproj=${kproj},w=${w},m=${m},probes=${probes}" >&2
        fi
      done
    done
  done
done

echo "Done. CSV: $CSV" >&2
