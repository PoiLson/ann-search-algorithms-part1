#!/bin/bash
# Quick LSH Parameter Tester
# Usage: ./quick_test.sh

echo "╔═══════════════════════════════════════════════════╗"
echo "║   LSH Quick Parameter Test for 2D Dataset         ║"
echo "╚═══════════════════════════════════════════════════╝"
echo ""

# Test 1: Optimal for 2D
echo "🔬 Test 1: OPTIMAL for 2D (High Recall)"
echo "   Parameters: K=3, L=20, W=6"
echo "   Expected: 90-100% recall"
echo "   ─────────────────────────────────────"
make all ALGO=lsh K=3 L=20 W=6 > /dev/null 2>&1
RECALL1=$(grep "Recall@N:" output.txt | awk '{print $2}')
QPS1=$(grep "QPS:" output.txt | awk '{print $2}')
echo "   ✓ Recall: $RECALL1"
echo "   ✓ QPS: $QPS1"
echo ""

# Test 2: Balanced
echo "🔬 Test 2: BALANCED (Speed vs Accuracy)"
echo "   Parameters: K=4, L=12, W=5"
echo "   Expected: 60-80% recall, faster"
echo "   ─────────────────────────────────────"
make all ALGO=lsh K=4 L=12 W=5 > /dev/null 2>&1
RECALL2=$(grep "Recall@N:" output.txt | awk '{print $2}')
QPS2=$(grep "QPS:" output.txt | awk '{print $2}')
echo "   ✓ Recall: $RECALL2"
echo "   ✓ QPS: $QPS2"
echo ""

# Test 3: Fast
echo "🔬 Test 3: FAST (Lower Recall)"
echo "   Parameters: K=5, L=8, W=4"
echo "   Expected: 40-60% recall, fastest"
echo "   ─────────────────────────────────────"
make all ALGO=lsh K=5 L=8 W=4 > /dev/null 2>&1
RECALL3=$(grep "Recall@N:" output.txt | awk '{print $2}')
QPS3=$(grep "QPS:" output.txt | awk '{print $2}')
echo "   ✓ Recall: $RECALL3"
echo "   ✓ QPS: $QPS3"
echo ""

echo "═══════════════════════════════════════════════════"
echo "📊 SUMMARY"
echo "═══════════════════════════════════════════════════"
echo ""
printf "%-20s | %-10s | %-12s | %s\n" "Configuration" "Recall" "QPS" "Recommendation"
echo "──────────────────────────────────────────────────────────────────"
printf "%-20s | %-10s | %-12s | %s\n" "K=3,L=20,W=6" "$RECALL1" "$QPS1" "✓ Best for accuracy"
printf "%-20s | %-10s | %-12s | %s\n" "K=4,L=12,W=5" "$RECALL2" "$QPS2" "✓ Balanced"
printf "%-20s | %-10s | %-12s | %s\n" "K=5,L=8,W=4" "$RECALL3" "$QPS3" "Fast queries"
echo ""
echo "💡 Tips:"
echo "   • For mostly correct results (>90%), use K=3, L≥15, W≥5"
echo "   • Lower K for lower dimensions (2D-10D)"
echo "   • Increase L to improve recall at cost of speed"
echo "   • Increase W to widen buckets and catch more neighbors"
echo ""
