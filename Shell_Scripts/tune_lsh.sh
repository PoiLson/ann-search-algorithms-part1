#!/bin/bash
# LSH Parameter Tuning Script
# This script tests different parameter combinations to find optimal recall

echo "LSH Parameter Tuning for 2D dataset"
echo "===================================="
echo ""

# Create results file
RESULTS_FILE="tuning_results.txt"
echo "K,L,W,Recall,AF,QPS" > $RESULTS_FILE

# Test different parameter combinations
# L (number of tables): more tables = higher recall but slower
# K (hash functions per table): balance between selectivity and coverage
# W (bucket width): larger = more points per bucket

echo "Testing parameter combinations..."
echo ""

# For 2D data with 100 points, good starting ranges:
# K: 3-6 (lower for low dimensions)
# L: 10-30 (more tables for better recall)
# W: 4-10 (larger for more coverage)

for L in 10 15 20 25; do
    for K in 3 4 5; do
        for W in 4 6 8; do
            echo -n "Testing K=$K, L=$L, W=$W ... "
            
            # Run the search with these parameters
            make run ALGO=lsh K=$K L=$L W=$W INPUT_FILE=random_2d_points.txt QUERY_FILE=query.dat OUTPUT_FILE=output.txt N=3 > /dev/null 2>&1
            
            # Extract metrics from output.txt
            if [ -f output.txt ]; then
                RECALL=$(grep "Recall@N:" output.txt | head -1 | awk '{print $2}')
                AF=$(grep "Average AF:" output.txt | head -1 | awk '{print $3}')
                QPS=$(grep "QPS:" output.txt | head -1 | awk '{print $2}')
                
                echo "$K,$L,$W,$RECALL,$AF,$QPS" >> $RESULTS_FILE
                echo "Recall=$RECALL, AF=$AF"
            else
                echo "FAILED"
            fi
        done
    done
done

echo ""
echo "Tuning complete! Results saved to $RESULTS_FILE"
echo ""
echo "Top 5 configurations by Recall:"
echo "================================"
sort -t',' -k4 -nr $RESULTS_FILE | head -6 | column -t -s','

echo ""
echo "Recommendations:"
echo "- Higher Recall = better accuracy"
echo "- Lower AF (Approximation Factor) = closer to optimal distance"
echo "- Higher QPS = faster queries"
