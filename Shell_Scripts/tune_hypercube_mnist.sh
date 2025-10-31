#!/usr/bin/env bash
set -euo pipefail

# Hypercube Parameter Tuning Script (MNIST)
# Same format as existing tuning scripts, but targeted for MNIST and Hypercube

OUT_DIR="runs/hypercube_mnist"
mkdir -p "$OUT_DIR"

RESULTS_FILE="$OUT_DIR/hypercube_mnist_tuning.csv"
: > "$RESULTS_FILE"
echo "KPROJ,W,M,PROBES,AvgRecall,AvgAF,QPS,OutFile" >> "$RESULTS_FILE"

# Parameter ranges to sweep
# - kproj in [11, 16]
# - w same as LSH grid: [30,35,40,45,50,55,60,80,100,120]
# - m in {10, 50, 100, 500, 1000, 2000, 4000, 8000}
# - probes in {1,2,5,10,50,100,500,800}
KPROJ_LIST=(11 16)
W_LIST=(30 35 40 45 50 55 60 80 100 120)
M_LIST=(10 50 100 500 1000 2000 4000 8000)
PROBES_LIST=(1 2 5 10 50 100 500 800)

# Base args for Makefile run (MNIST, N=1, RANGE=false)
BASE_ARGS="ALGO=hypercube TYPE=mnist RANGE=false N=1 INPUT_FILE=Data/MNIST/train-images.idx3-ubyte QUERY_FILE=Data/MNIST/t10k-images-100-sample.idx3-ubyte"

run_combo() {
	local kproj=$1 w=$2 m=$3 probes=$4
	local tag="k${kproj}_w${w}_m${m}_p${probes}"
	local outfile="$OUT_DIR/run_${tag}.txt"

	echo "Running KPROJ=$kproj W=$w M=$m PROBES=$probes -> $outfile" >&2
	make -s $BASE_ARGS OUTPUT_FILE="$outfile" KPROJ="$kproj" HYPERCUBE_W="$w" M="$m" PROBES="$probes" > /dev/null 2>&1 || true

	if [[ -f "$outfile" ]]; then
		local recall af qps
		recall=$(grep -F "Average Recall@N:" "$outfile" | tail -1 | awk '{print $3}')
		af=$(grep -F "Average AF (mean over queries):" "$outfile" | tail -1 | awk '{print $6}')
		qps=$(grep -F "QPS_overall:" "$outfile" | tail -1 | awk '{print $2}')
		recall=${recall:-0}; af=${af:-nan}; qps=${qps:-0}
		echo "$kproj,$w,$m,$probes,$recall,$af,$qps,$outfile" >> "$RESULTS_FILE"
		echo "  -> AvgRecall=$recall AF=$af QPS=$qps" >&2
	else
		echo "$kproj,$w,$m,$probes,0,nan,0,$outfile" >> "$RESULTS_FILE"
		echo "  -> missing output: $outfile" >&2
	fi
}

# Full sweep per request
for kproj in "${KPROJ_LIST[@]}"; do
	for w in "${W_LIST[@]}"; do
		for m in "${M_LIST[@]}"; do
			for probes in "${PROBES_LIST[@]}"; do
				run_combo "$kproj" "$w" "$m" "$probes"
			done
		done
	done
done

{
	echo "\nTop 20 by AvgRecall (desc):"
	sort -t',' -k5,5nr -k6,6n "$RESULTS_FILE" | head -n 21 | column -t -s','
} >&2

echo "\nDone. Results in $RESULTS_FILE and per-run outputs in $OUT_DIR" >&2

