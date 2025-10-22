#!/usr/bin/env bash
set -euo pipefail

RESULTS_FILE="mnist_k_sweep.csv"
: > "$RESULTS_FILE"
echo "K,AvgRecall,AvgAF,QPS" >> "$RESULTS_FILE"

# Fixed params
L=30
W=2000

BASE_ARGS="ALGO=lsh INPUT_FILE=Mnist_data/train-images.idx3-ubyte QUERY_FILE=Mnist_data/t10k-images-100-sample.idx3-ubyte OUTPUT_FILE=output.txt TYPE=mnist RANGE=false"

run_k() {
  local k=$1
  echo "Running K=$k L=$L W=$W" >&2
  make -s $BASE_ARGS K=$k L=$L W=$W > /dev/null 2>&1 || true
  if [[ -f output.txt ]]; then
    local recall af qps
    recall=$(grep -F "Average Recall@N:" output.txt | tail -1 | awk '{print $3}')
    af=$(grep -F "Average AF (mean over queries):" output.txt | tail -1 | awk '{print $6}')
    qps=$(grep -F "QPS_overall:" output.txt | tail -1 | awk '{print $2}')
    recall=${recall:-0}; af=${af:-nan}; qps=${qps:-0}
    echo "$k,$recall,$af,$qps" >> "$RESULTS_FILE"
    echo "  -> AvgRecall=$recall AF=$af QPS=$qps" >&2
  else
    echo "$k,0,nan,0" >> "$RESULTS_FILE"
    echo "  -> output.txt missing" >&2
  fi
}

for k in $(seq 5 14); do
  run_k "$k"
done

{
  echo "\nTop 10 by AvgRecall (desc), AF as tiebreaker:";
  sort -t',' -k2,2nr -k3,3n "$RESULTS_FILE" | head -n 11 | column -t -s','
} >&2

echo "\nDone. Results in $RESULTS_FILE" >&2
