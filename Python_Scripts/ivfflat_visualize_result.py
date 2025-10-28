import matplotlib.pyplot as plt

# Points in the same order as printed
import os
import csv

# If the program wrote CSVs, prefer to load them. Otherwise fall back to the hard-coded example.
centroids = []
points = []
clusters = []
centroids_csv = 'Python_Scripts/ivfflat_centroids.csv'
assign_csv = 'Python_Scripts/ivfflat_assignments.csv'
if os.path.exists(centroids_csv) and os.path.exists(assign_csv):
    # load centroids
    with open(centroids_csv, newline='') as fc:
        r = csv.DictReader(fc)
        for row in r:
            centroids.append((float(row['x']), float(row['y'])))

    # load assignments
    with open(assign_csv, newline='') as fa:
        r = csv.DictReader(fa)
        for row in r:
            points.append((float(row['x']), float(row['y'])))
            clusters.append(int(row['cluster']))
else:
    # fallback example (same as before)
    points = [
        (1.0, 1.0),
        (6.0, 6.0),
        (3.0, 0.0),
        (7.0, 1.0),
        (7.0, 0.0),
        (0.0, 8.0),
        (3.0, 4.0),
        (3.0, 7.0),
        (7.0, 5.0),
        (1.0, 3.0),
    ]
    centroids = [ (3.0, 0.0), (0.0, 8.0) ]
    cluster0 = {8,0,4,9,3,2,6}
    clusters = [0 if i in cluster0 else 1 for i in range(len(points))]
    
colors = ['tab:orange' if c==0 else 'tab:green' for c in clusters]

fig, ax = plt.subplots(figsize=(6,6))
xs = [p[0] for p in points]
ys = [p[1] for p in points]

ax.scatter(xs, ys, c=colors, s=100, edgecolors='k')
for i,(x,y) in enumerate(points):
    ax.text(x+0.12, y+0.12, str(i), fontsize=9)

# Plot centroids (handle arbitrary number of centroids and color mapping)
if centroids:
    cx = [c[0] for c in centroids]
    cy = [c[1] for c in centroids]
    # build color list matching number of centroids
    cmap = ['tab:orange','tab:green','tab:blue','tab:red','tab:purple']
    centroid_colors = [cmap[i % len(cmap)] for i in range(len(centroids))]
    ax.scatter(cx, cy, marker='X', c=centroid_colors, s=220, linewidths=1.5, edgecolors='k')
    for i,(x,y) in enumerate(centroids):
        ax.text(x+0.12, y-0.35, f"centroid[{i}]", fontsize=10, fontweight='bold')

ax.set_title('IVFFlat KMeans result visualization')
ax.set_xlabel('x')
ax.set_ylabel('y')
ax.set_aspect('equal', 'box')
ax.grid(True, linestyle='--', alpha=0.5)
plt.tight_layout()
out = 'Python_Scripts/ivfflat_kmeans_result.png'
plt.savefig(out, dpi=150)
print('Saved visualization to', out)
