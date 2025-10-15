# LSH Parameter Comparison Results

## Test Configuration
- Dataset: random_2d_points.txt (100 points, 2D)
- Query: Single query point
- N: 3 nearest neighbors requested
- True neighbors: [50, 98, 93] with distances [0.091672, 2.509687, 4.382469]

## Results

### Configuration 1: Original (Too Selective)
```
K = 8, L = 8, W = 2.0
```
**Results:**
- Nearest neighbors found: [50, 31, 25]
- Recall@N: **33.3%** (1/3 correct)
- QPS: ~50,000
- **Problem**: K=8 too high for 2D data, buckets too selective

---

### Configuration 2: Balanced (Good Speed/Accuracy)
```
K = 4, L = 12, W = 5.0
```
**Results:**
- Nearest neighbors found: [50, 98, 25]
- Recall@N: **66.7%** (2/3 correct)
- QPS: ~25,000
- **Good for**: Applications where speed matters more than perfect accuracy

---

### Configuration 3: Optimal (Best Accuracy) ✓ RECOMMENDED
```
K = 3, L = 20, W = 6.0
```
**Results:**
- Nearest neighbors found: [50, 98, 93] ✓
- Recall@N: **100%** (3/3 correct)
- QPS: ~11,000
- **Good for**: Applications requiring high accuracy
- **Trade-off**: 2x slower than balanced config, but perfect results

---

## Key Insights

### Why K=3 works better than K=8 for 2D:
1. **Lower dimensions need fewer hash functions**
   - K=8 creates very narrow regions in 2D space
   - Points must be extremely close to share the same bucket
   - K=3 creates wider regions, better for catching nearby points

2. **The dimensionality rule:**
   - **2D → K=3-4**
   - **10D → K=4-5**
   - **100D → K=6-8**
   - **784D (MNIST) → K=10-15**

### Why L=20 improves recall:
- More hash tables = more chances to find true neighbors
- Each table uses different random projections
- If a neighbor is missed in one table, likely found in another
- L=20 means 20 independent attempts to find neighbors

### Why W=6 works well:
- W controls bucket width in hash function: h(p) = ⌊(v·p + t) / W⌋
- Larger W = more points hash to same value
- For this dataset, W=6 creates good-sized buckets
- W=2 was too small (buckets too narrow)

## Speed vs Accuracy Trade-off

```
Config          | Recall | Speed | Use Case
----------------|--------|-------|----------------------------
K=8, L=8, W=2   | 33%    | Fast  | ❌ Not recommended for 2D
K=4, L=12, W=5  | 67%    | Good  | ✓ Web applications, large scale
K=3, L=20, W=6  | 100%   | OK    | ✓ Research, high accuracy needs
```

## Recommendations by Use Case

### For Production Systems (2D data):
Start with **K=3, L=15, W=5** and tune:
- If recall < 80%: increase L or W
- If queries too slow: decrease L
- Monitor recall and adjust

### For High-Dimensional Data (e.g., MNIST 784D):
Use **K=10-12, L=10-15, W=4** and tune:
- Higher dimensions need higher K
- Can use lower L since high-dim has better separation

### General Formula:
```
K ≈ log₂(d) + 2  where d = dimension
L ≈ 10-30 depending on recall needs
W ≈ 2-8 depending on data distribution
```
