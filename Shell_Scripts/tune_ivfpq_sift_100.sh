#!/usr/bin/env bash
set -euo pipefail

# IVFPQ Parameter Tuning Script (SIFT, 100 queries)
# Grid: kclusters={10,20,50,60}, nprobe={5,20,40}, nbits={8,16}, M={16,32,64,128}

OUT_DIR="runs/ivfpq_sift_100"
mkdir -p "$OUT_DIR"
CSV="$OUT_DIR/ivfpq_sift_100_grid.csv"
: > "$CSV"
echo "KCLUSTERS,NPROBE,M,NBITS,AvgRecall,AvgAF,AvgTApprox,AvgTTrue,QPS" >> "$CSV"

KCLUSTERS=(10 20 50 60)
NPROBES=(5 20 40)
MBLOCKS=(16 32 64 128)
NBITS_LIST=(8)

DATASET="Data/SIFT/sift_base.fvecs"
QUERIES="Data/SIFT/sift_query_100.fvecs"

for kclusters in "${KCLUSTERS[@]}"; do
  for nprobe in "${NPROBES[@]}"; do
    for M in "${MBLOCKS[@]}"; do
      for nbits in "${NBITS_LIST[@]}"; do
        echo "Running IVFPQ (SIFT-100): kclusters=$kclusters nprobe=$nprobe M=$M nbits=$nbits" >&2
        tmpf=$(mktemp)
        if ! ./search -d "$DATASET" -q "$QUERIES" -ivfpq -kclusters "$kclusters" -nprobe "$nprobe" -M "$M" -nbits "$nbits" -o "$tmpf" -N 1 -R 50000 -type sift -range false -seed 42 >/dev/null 2>&1; then
          echo "search returned non-zero for kclusters=$kclusters nprobe=$nprobe M=$M nbits=$nbits" >&2
        fi

        if [[ -f "$tmpf" ]]; then
          recall=$(grep -F "Recall@N:" "$tmpf" | tail -1 | sed -E 's/.*: *([0-9.+-eE]+|inf|nan).*/\1/' || echo 0)
          af=$(grep -F "Average AF:" "$tmpf" | tail -1 | sed -E 's/.*: *([0-9.+-eE]+|inf|nan).*/\1/' || echo nan)
          tapprox=$(grep -F "tApproximateAverage:" "$tmpf" | tail -1 | sed -E 's/.*: *([0-9.+-eE]+|inf|nan).*/\1/' || echo 0)
          ttrue=$(grep -F "tTrueAverage:" "$tmpf" | tail -1 | sed -E 's/.*: *([0-9.+-eE]+|inf|nan).*/\1/' || echo 0)
          qps=$(grep -F "QPS:" "$tmpf" | tail -1 | sed -E 's/.*: *([0-9.+-eE]+|inf|nan).*/\1/' || echo 0)
          recall=${recall:-0}; af=${af:-nan}; tapprox=${tapprox:-0}; ttrue=${ttrue:-0}; qps=${qps:-0}
          echo "${kclusters},${nprobe},${M},${nbits},${recall},${af},${tapprox},${ttrue},${qps}" >> "$CSV"
          rm -f "$tmpf"
          echo "Appended kclusters=$kclusters,nprobe=$nprobe,M=$M,nbits=$nbits -> recall=${recall} af=${af} tApprox=${tapprox} tTrue=${ttrue} qps=${qps}" >&2
        else
          echo "${kclusters},${nprobe},${M},${nbits},0,nan,0,0,0" >> "$CSV"
          echo "Missing output for kclusters=${kclusters},nprobe=${nprobe},M=${M},nbits=${nbits}" >&2
        fi
      done
    done
  done
done

echo "Done. CSV: $CSV" >&2
