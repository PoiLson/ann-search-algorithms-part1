import matplotlib.pyplot as plt

# Data points (x, y)
points = [
    (3.000000, 6.000000),
    (7.000000, 5.000000),
    (3.000000, 5.000000),
    (6.000000, 2.000000),
    (9.000000, 1.000000),
    (2.000000, 7.000000),
    (0.000000, 9.000000),
    (3.000000, 6.000000),
    (0.000000, 6.000000),
    (2.000000, 6.000000)
]

# Centroids
centroids = [
    (0.000000, 6.000000),
    (9.000000, 1.000000)
]

# Separate x and y coordinates
x_points, y_points = zip(*points)
x_centroids, y_centroids = zip(*centroids)

# Plot all points
plt.scatter(x_points, y_points, color='blue', label='Data Points', s=50)

# Plot centroids
plt.scatter(x_centroids, y_centroids, color='red', label='Centroids', s=200, marker='X')

# Add labels for centroids
for i, (x, y) in enumerate(centroids):
    plt.text(x + 0.1, y + 0.1, f'C{i}', fontsize=12, color='red')

# Styling
plt.title('Points and Centroids')
plt.xlabel('X coordinate')
plt.ylabel('Y coordinate')
plt.legend()
plt.grid(True)
plt.axis('equal')

plt.show()

# Save figure
output_path = "centroids_plot.png"
plt.savefig(output_path, dpi=300, bbox_inches='tight')
print(f"Plot saved as {output_path}")