#!/usr/bin/env bash
set -euo pipefail

OUT_DIR="runs/ivfflat_mnist"
mkdir -p "$OUT_DIR"
CSV="$OUT_DIR/ivfflat_mnist_tuning.csv"
: > "$CSV"
echo "KCLUSTERS,NPROBES,AvgRecall,AvgAF,QPS" >> "$CSV"

KLIST=(35 45 50 60 65 80)
NPROBES=(2 3 4 5 6)

BASE_ARGS="ALGO=ivfflat TYPE=mnist RANGE=false N=1 INPUT_FILE=Data/MNIST/train-images.idx3-ubyte QUERY_FILE=Data/MNIST/t10k-images-100-sample.idx3-ubyte"

for k in "${KLIST[@]}"; do
  for n in "${NPROBES[@]}"; do
    echo "Running ivfflat: KCLUSTERS=$k NPROBE=$n" >&2
    tmpf=$(mktemp)
    # Run make and capture full output temporarily
    make -s $BASE_ARGS OUTPUT_FILE="$tmpf" KCLUSTERS="$k" NPROBE="$n" || echo "make returned non-zero for k=$k n=$n" >&2

    if [[ -f "$tmpf" ]]; then
      recall=$(grep -F "Average Recall@N:" "$tmpf" | tail -1 | sed -E 's/.*: *([0-9.]+).*/\1/' || echo 0)
      af=$(grep -F "Average AF (mean over queries):" "$tmpf" | tail -1 | sed -E 's/.*: *([0-9.]+).*/\1/' || echo nan)
      qps=$(grep -F "QPS_overall:" "$tmpf" | tail -1 | sed -E 's/.*: *([0-9.]+).*/\1/' || echo 0)
      recall=${recall:-0}; af=${af:-nan}; qps=${qps:-0}
      echo "${k},${n},${recall},${af},${qps}" >> "$CSV"
      rm -f "$tmpf"
      echo "Appended K=$k,N=$n -> recall=${recall} af=${af} qps=${qps}" >&2
    else
      echo "${k},${n},0,nan,0" >> "$CSV"
      echo "Missing output for k=${k},n=${n}" >&2
    fi
  done
done

echo "Done. CSV: $CSV" >&2
