#!/usr/bin/env bash
set -euo pipefail

RESULTS_FILE="mnist_kw_sweep.csv"
: > "$RESULTS_FILE"
echo "K,W,AvgRecall,AvgAF,QPS" >> "$RESULTS_FILE"

# Fixed param
L=30

K_LIST=(8 9 10 11 12)
W_LIST=(10 20 30 40 50)

BASE_ARGS="ALGO=lsh INPUT_FILE=Mnist_data/train-images.idx3-ubyte QUERY_FILE=Mnist_data/t10k-images-100-sample.idx3-ubyte OUTPUT_FILE=output.txt TYPE=mnist RANGE=false"

run_combo() {
  local k=$1 w=$2
  echo "Running K=$k L=$L W=$w" >&2
  make -s $BASE_ARGS K=$k L=$L W=$w > /dev/null 2>&1 || true
  if [[ -f output.txt ]]; then
    local recall af qps
    recall=$(grep -F "Average Recall@N:" output.txt | tail -1 | awk '{print $3}')
    af=$(grep -F "Average AF (mean over queries):" output.txt | tail -1 | awk '{print $6}')
    qps=$(grep -F "QPS_overall:" output.txt | tail -1 | awk '{print $2}')
    recall=${recall:-0}; af=${af:-nan}; qps=${qps:-0}
    echo "$k,$w,$recall,$af,$qps" >> "$RESULTS_FILE"
    echo "  -> AvgRecall=$recall AF=$af QPS=$qps" >&2
  else
    echo "$k,$w,0,nan,0" >> "$RESULTS_FILE"
    echo "  -> output.txt missing" >&2
  fi
}

for k in "${K_LIST[@]}"; do
  for w in "${W_LIST[@]}"; do
    run_combo "$k" "$w"
  done
done

{
  echo "\nTop 10 by AvgRecall (desc), AF as tiebreaker:";
  sort -t',' -k3,3nr -k4,4n "$RESULTS_FILE" | head -n 11 | column -t -s','
} >&2

echo "\nDone. Results in $RESULTS_FILE" >&2
