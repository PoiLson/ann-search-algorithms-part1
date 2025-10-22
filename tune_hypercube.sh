#!/usr/bin/env bash
set -euo pipefail

RESULTS_FILE="hypercube_tuning.csv"
: > "$RESULTS_FILE"
echo "KPROJ,W,M,PROBES,AvgRecall,AvgAF,QPS" >> "$RESULTS_FILE"

# Parameter ranges to sweep
KPROJ_LIST=(3 4 5 6)
W_LIST=(1000 1500 2000 2500 3000)
M_LIST=(10 20 50)
PROBES_LIST=(2 5 10 15)

BASE_ARGS="ALGO=hypercube INPUT_FILE=Mnist_data/train-images.idx3-ubyte QUERY_FILE=Mnist_data/t10k-images-100-sample.idx3-ubyte OUTPUT_FILE=output.txt TYPE=mnist RANGE=false N=5"

run_combo() {
  local kproj=$1 w=$2 m=$3 probes=$4
  echo "Running KPROJ=$kproj W=$w M=$m PROBES=$probes" >&2
  make -s $BASE_ARGS KPROJ=$kproj HYPERCUBE_W=$w M=$m PROBES=$probes > /dev/null 2>&1 || true
  
  if [[ -f output.txt ]]; then
    local recall af qps
    recall=$(grep -F "Average Recall@N:" output.txt | tail -1 | awk '{print $3}')
    af=$(grep -F "Average AF (mean over queries):" output.txt | tail -1 | awk '{print $6}')
    qps=$(grep -F "QPS_overall:" output.txt | tail -1 | awk '{print $2}')
    recall=${recall:-0}; af=${af:-nan}; qps=${qps:-0}
    echo "$kproj,$w,$m,$probes,$recall,$af,$qps" >> "$RESULTS_FILE"
    echo "  -> AvgRecall=$recall AF=$af QPS=$qps" >&2
  else
    echo "$kproj,$w,$m,$probes,0,nan,0" >> "$RESULTS_FILE"
    echo "  -> output.txt missing" >&2
  fi
}

# Quick focused sweep: vary PROBES and W primarily (most impactful for recall)
# Fix KPROJ=4, vary M moderately
for kproj in 4; do
  for w in "${W_LIST[@]}"; do
    for m in 10 50; do
      for probes in "${PROBES_LIST[@]}"; do
        run_combo "$kproj" "$w" "$m" "$probes"
      done
    done
  done
done

# Test a few KPROJ variations with best W/M/PROBES from above
echo "Testing KPROJ variations..." >&2
for kproj in 3 5 6; do
  for w in 2000 2500; do
    run_combo "$kproj" "$w" 50 15
  done
done

{
  echo "\nTop 15 by AvgRecall (desc):"
  sort -t',' -k5,5nr -k6,6n "$RESULTS_FILE" | head -n 16 | column -t -s','
} >&2

echo "\nDone. Results in $RESULTS_FILE" >&2
