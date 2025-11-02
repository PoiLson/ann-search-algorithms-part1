#!/usr/bin/env bash
set -euo pipefail

OUT_DIR="runs/ivfflat_mnist"
mkdir -p "$OUT_DIR"
CSV="$OUT_DIR/ivfflat_mnist_100_grid.csv"
: > "$CSV"
echo "KCLUSTERS,NPROBE,AvgRecall,AvgAF,AvgTApprox,AvgTTrue,QPS" >> "$CSV"

KCLUSTERS=(10 20 40 50 60 80)
NPROBES=(2 3 4 5)

DATASET="Data/MNIST/train-images.idx3-ubyte"
QUERIES="Data/MNIST/t10k-images-100-sample.idx3-ubyte"

for kclusters in "${KCLUSTERS[@]}"; do
  for nprobe in "${NPROBES[@]}"; do
    echo "Running IVFFlat (MNIST-100): kclusters=$kclusters nprobe=$nprobe" >&2
    tmpf=$(mktemp)
    if ! ./search -d "$DATASET" -q "$QUERIES" -ivfflat -kclusters "$kclusters" -nprobe "$nprobe" -M 98 -o "$tmpf" -N 1 -R 50000 -type mnist -range false >/dev/null 2>&1; then
      echo "search returned non-zero for kclusters=$kclusters nprobe=$nprobe" >&2
    fi

    if [[ -f "$tmpf" ]]; then
      recall=$(grep -F "Recall@N:" "$tmpf" | tail -1 | sed -E 's/.*: *([0-9.+-eE]+|inf|nan).*/\1/' || echo 0)
      af=$(grep -F "Average AF:" "$tmpf" | tail -1 | sed -E 's/.*: *([0-9.+-eE]+|inf|nan).*/\1/' || echo nan)
      tapprox=$(grep -F "tApproximateAverage:" "$tmpf" | tail -1 | sed -E 's/.*: *([0-9.+-eE]+|inf|nan).*/\1/' || echo 0)
      ttrue=$(grep -F "tTrueAverage:" "$tmpf" | tail -1 | sed -E 's/.*: *([0-9.+-eE]+|inf|nan).*/\1/' || echo 0)
      qps=$(grep -F "QPS:" "$tmpf" | tail -1 | sed -E 's/.*: *([0-9.+-eE]+|inf|nan).*/\1/' || echo 0)
      recall=${recall:-0}; af=${af:-nan}; tapprox=${tapprox:-0}; ttrue=${ttrue:-0}; qps=${qps:-0}
      echo "${kclusters},${nprobe},${recall},${af},${tapprox},${ttrue},${qps}" >> "$CSV"
      rm -f "$tmpf"
      echo "Appended kclusters=$kclusters,nprobe=$nprobe -> recall=${recall} af=${af} tApprox=${tapprox} tTrue=${ttrue} qps=${qps}" >&2
    else
      echo "${kclusters},${nprobe},0,nan,0,0,0" >> "$CSV"
      echo "Missing output for kclusters=${kclusters},nprobe=${nprobe}" >&2
    fi
  done
done

echo "Done. CSV: $CSV" >&2
