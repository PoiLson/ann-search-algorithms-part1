#!/usr/bin/env bash
set -euo pipefail

# Hypercube Parameter Tuning Script (MNIST, Custom Grid)
# Sweeps kproj={12,13,14}, w={40,60,80,90}, M={5000,8500}, probes={500,850}

OUT_DIR="runs/hypercube_mnist"
mkdir -p "$OUT_DIR"
CSV="$OUT_DIR/hypercube_mnist_custom_grid.csv"
: > "$CSV"
echo "KPROJ,W,M,PROBES,AvgRecall,AvgAF,AvgTApprox,AvgTTrue,QPS" >> "$CSV"

KPROJ_LIST=(12 13 14)
W_LIST=(40 60 80 90)
M_LIST=(5000 8500)
PROBES_LIST=(500 850)

for kproj in "${KPROJ_LIST[@]}"; do
  for w in "${W_LIST[@]}"; do
    for m in "${M_LIST[@]}"; do
      for probes in "${PROBES_LIST[@]}"; do
        echo "Running Hypercube (MNIST-100): kproj=$kproj w=$w m=$m probes=$probes" >&2
        tmpf=$(mktemp)
        if ! ./search -d Data/MNIST/train-images.idx3-ubyte -q Data/MNIST/t10k-images-100-sample.idx3-ubyte -kproj "$kproj" -w "$w" -M "$m" -probes "$probes" -N 1 -R 4 -type mnist -range false -hypercube -o "$tmpf" >/dev/null 2>&1; then
          echo "search returned non-zero for kproj=$kproj w=$w m=$m probes=$probes" >&2
        fi

        if [[ -f "$tmpf" ]]; then
  recall=$(grep -F "Average Recall@N:" "$tmpf" | tail -1 | sed -E 's/.*: *([0-9.+-eE]+|inf|nan).*/\1/' || echo 0)
  af=$(grep -F "Average AF (mean over queries):" "$tmpf" | tail -1 | sed -E 's/.*: *([0-9.+-eE]+|inf|nan).*/\1/' || echo nan)
  tapprox=$(grep -F "Average tApproximate:" "$tmpf" | tail -1 | sed -E 's/.*: *([0-9.+-eE]+|inf|nan).*/\1/' || echo 0)
  ttrue=$(grep -F "Average tTrue:" "$tmpf" | tail -1 | sed -E 's/.*: *([0-9.+-eE]+|inf|nan).*/\1/' || echo 0)
  qps=$(grep -F "QPS_overall:" "$tmpf" | tail -1 | sed -E 's/.*: *([0-9.+-eE]+|inf|nan).*/\1/' || echo 0)
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
