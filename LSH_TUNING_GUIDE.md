# LSH Parameter Tuning Guide

## Understanding LSH Parameters

### 1. **L (Number of Hash Tables)**
- **Higher L → Better Recall** (more chances to find true neighbors)
- **Higher L → Slower queries** (more tables to search)
- **Typical range**: 10-50 for good recall
- **Current**: L=8 (too low for high recall)

### 2. **k (Hash Functions per Table)**
- **Higher k → More selective** (fewer collisions, smaller buckets)
- **Lower k → More coverage** (more points in same bucket)
- **For 2D data**: k=3-5 is usually good
- **For high-dimensional data**: k=8-15
- **Current**: k=8 (too high for 2D data!)

### 3. **w (Bucket Width)**
- **Larger w → Wider buckets** (more points hash to same value)
- **Smaller w → Narrower buckets** (more selective)
- **Rule of thumb**: w ≈ 2-4 times the typical distance between neighbors
- **Current**: w=2.0 (might be too small)

## Problem Analysis

Your current setup:
- **Dataset**: 2D, 100 points
- **Parameters**: K=8, L=8, W=2.0
- **Recall**: 33% (1 out of 3 neighbors correct)

**Issues:**
1. **K=8 is too high for 2D data** → buckets are too selective, missing neighbors
2. **L=8 is moderate** → could be increased for better recall
3. **W=2.0 might be too small** → buckets are narrow

## Recommended Parameter Sets

### For High Recall (90%+):
```bash
K = 3
L = 25
W = 6.0
```
- **Pros**: Very high probability of finding true neighbors
- **Cons**: Slower queries (checking more tables)

### For Balanced (70-80% recall):
```bash
K = 4
L = 15
W = 5.0
```
- **Pros**: Good balance of speed and accuracy
- **Cons**: May miss some neighbors

### For Fast Queries (50-60% recall):
```bash
K = 5
L = 10
W = 4.0
```
- **Pros**: Faster queries
- **Cons**: Lower recall

## Why K is Critical for 2D Data

In 2D space:
- With K=8 hash functions, you're slicing the space into many narrow regions
- Points must be very close to hash to the same ID
- This is too restrictive for low dimensions

**The curse of dimensionality works backwards here:**
- Low dimensions (2D) → Use low K (3-5)
- High dimensions (784D for MNIST) → Use high K (8-15)

## Quick Test Commands

Test high recall setup:
```bash
make run ALGO=lsh K=3 L=25 W=6
```

Test balanced setup:
```bash
make run ALGO=lsh K=4 L=15 W=5
```

Test current setup for comparison:
```bash
make run ALGO=lsh K=8 L=8 W=2
```

## How to Choose Parameters

1. **Start with dimension-based K:**
   - 2D: K=3
   - 3D-10D: K=4-5
   - 50D+: K=6-8
   - 500D+: K=10-15

2. **Set L based on recall needs:**
   - Need 90%+ recall: L=20-30
   - Need 70-80% recall: L=10-15
   - Need 50-60% recall: L=5-10

3. **Tune W experimentally:**
   - Start with W = 4
   - If recall too low, increase W
   - If too many candidates, decrease W

## Monitoring Results

Good indicators:
- **Recall@N**: Percentage of true neighbors found (aim for >0.7)
- **AF (Approximation Factor)**: Distance ratio (closer to 1.0 is better)
- **QPS (Queries Per Second)**: Speed metric

## Advanced: Table Size Tuning

Currently hardcoded:
```c
lsh->table_size = 25;        // Buckets per table
lsh->num_of_buckets = 91;    // For ID calculation
```

For better distribution:
- `table_size` should be ≈ n/4 to n/2 (where n = dataset size)
- `num_of_buckets` should be a prime number ≥ table_size * 2
- For 100 points: table_size=25-50, num_of_buckets=61-101
