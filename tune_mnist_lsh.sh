#!/usr/bin/env bash
set -euo pipefail

RESULTS_FILE="mnist_lsh_tuning.csv"
: > "$RESULTS_FILE"
echo "K,L,W,AvgRecall,AvgAF,QPS" >> "$RESULTS_FILE"

# Sweeps
K_LIST=(4 5 6)
L_LIST=(40 60 80)
W_LIST=(500 1000 2000)

# Ensure we run LSH with MNIST inputs
BASE_ARGS="ALGO=lsh INPUT_FILE=Mnist_data/train-images.idx3-ubyte QUERY_FILE=Mnist_data/t10k-images-100-sample.idx3-ubyte OUTPUT_FILE=output.txt TYPE=mnist RANGE=false"

run_combo() {
  local k=$1 l=$2 w=$3
  echo "Running K=$k L=$l W=$w" >&2
  # Build and run quietly; Makefile's default target builds and runs
  make -s $BASE_ARGS K=$k L=$l W=$w > /dev/null 2>&1 || true

  if [[ -f output.txt ]]; then
    # Parse the OVERALL metrics at the end
    local recall af qps
    recall=$(grep -F "Average Recall@N:" output.txt | tail -1 | awk '{print $3}')
    af=$(grep -F "Average AF (mean over queries):" output.txt | tail -1 | awk '{print $6}')
    qps=$(grep -F "QPS_overall:" output.txt | tail -1 | awk '{print $2}')
    # Fallbacks if parsing failed
    recall=${recall:-0}
    af=${af:-nan}
    qps=${qps:-0}
    echo "$k,$l,$w,$recall,$af,$qps" >> "$RESULTS_FILE"
    echo "  -> AvgRecall=$recall AF=$af QPS=$qps" >&2
  else
    echo "$k,$l,$w,0,nan,0" >> "$RESULTS_FILE"
    echo "  -> output.txt missing" >&2
  fi
}

for k in "${K_LIST[@]}"; do
  for l in "${L_LIST[@]}"; do
    for w in "${W_LIST[@]}"; do
      run_combo "$k" "$l" "$w"
    done
  done
done

# Show top-5 by AvgRecall, then by AF ascending as tiebreaker
{
  echo "Top 5 by AvgRecall:\n"
  sort -t',' -k4,4nr -k5,5n "$RESULTS_FILE" | head -n 6 | column -t -s','
} >&2

echo "Done. Results in $RESULTS_FILE" >&2
