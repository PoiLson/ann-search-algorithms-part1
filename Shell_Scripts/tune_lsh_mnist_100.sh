#!/usr/bin/env bash
set -euo pipefail

OUT_DIR="runs/lsh_mnist_100"
mkdir -p "$OUT_DIR"
CSV="$OUT_DIR/lsh_mnist_100_grid.csv"
: > "$CSV"
echo "K,L,W,AvgRecall,AvgAF,AvgTApprox,AvgTTrue,QPS" >> "$CSV"

# Parameter grids
KS=(3 4 5 6)
LS=(5 10 15 20)
WS=(40 60 80 100 120)

DATASET="Data/MNIST/train-images.idx3-ubyte"
QUERIES="Data/MNIST/t10k-images-100-sample.idx3-ubyte"

for k in "${KS[@]}"; do
  for L in "${LS[@]}"; do
    for w in "${WS[@]}"; do
      echo "Running LSH: k=$k L=$L w=$w on MNIST-100" >&2
      tmpf=$(mktemp)
      if ! ./search -d "$DATASET" -q "$QUERIES" -k "$k" -L "$L" -w "$w" -o "$tmpf" -N 1 -R 5 -type mnist -lsh -range false >/dev/null 2>&1; then
        echo "search returned non-zero for k=$k L=$L w=$w" >&2
      fi

      if [[ -f "$tmpf" ]]; then
  recall=$(grep -F "Average Recall@N:" "$tmpf" | tail -1 | sed -E 's/.*: *([0-9.+-eE]+|inf|nan).*/\1/' || echo 0)
  af=$(grep -F "Average AF (mean over queries):" "$tmpf" | tail -1 | sed -E 's/.*: *([0-9.+-eE]+|inf|nan).*/\1/' || echo nan)
  tapprox=$(grep -F "Average tApproximate:" "$tmpf" | tail -1 | sed -E 's/.*: *([0-9.+-eE]+|inf|nan).*/\1/' || echo 0)
  ttrue=$(grep -F "Average tTrue:" "$tmpf" | tail -1 | sed -E 's/.*: *([0-9.+-eE]+|inf|nan).*/\1/' || echo 0)
  qps=$(grep -F "QPS_overall:" "$tmpf" | tail -1 | sed -E 's/.*: *([0-9.+-eE]+|inf|nan).*/\1/' || echo 0)
        recall=${recall:-0}; af=${af:-nan}; tapprox=${tapprox:-0}; ttrue=${ttrue:-0}; qps=${qps:-0}
        echo "${k},${L},${w},${recall},${af},${tapprox},${ttrue},${qps}" >> "$CSV"
        rm -f "$tmpf"
        echo "Appended k=$k,L=$L,w=$w -> recall=${recall} af=${af} tApprox=${tapprox} tTrue=${ttrue} qps=${qps}" >&2
      else
        echo "${k},${L},${w},0,nan,0,0,0" >> "$CSV"
        echo "Missing output for k=${k},L=${L},w=${w}" >&2
      fi
    done
  done
done

echo "Done. CSV: $CSV" >&2
