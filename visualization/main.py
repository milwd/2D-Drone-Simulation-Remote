#!/usr/bin/env python3
import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D

# --------------------------
# Domain and grid settings
# --------------------------
width = 30      # domain width
height = 20     # domain height
n_points = 100  # resolution for plotting

x = np.linspace(0, width, n_points)
y = np.linspace(0, height, n_points)
X, Y = np.meshgrid(x, y)

# --------------------------
# Define obstacles and targets
# --------------------------
# Example obstacles and targets (you can change or add more)
obstacles = [(5, 5), (15, 15), (10, 10)]
targets   = [(5, 15), (15, 5)]

# --------------------------
# Parameters for potential functions
# --------------------------
# Influence radius (for both repulsive and attractive fields)
d0_rep = 4.0   # repulsive influence radius
d0_att = 4.0   # attractive influence radius

# Maximum potential contribution (spike height)
U_rep_max = 200.0  # maximum repulsive potential (barrier)
U_att_max = 200.0  # maximum attractive potential (well); note: we subtract it later

# Fraction of d0 below which the potential saturates
saturation_frac = 0.1  # i.e. if d < 10% of d0, use the maximum

# --------------------------
# Define normalized (saturated) potential functions
# --------------------------
def normalized_potential(d, d0, U_max):
    """
    Computes a piecewise linear potential that is saturated at U_max for distances
    below d_thresh and decays linearly to 0 at d0.
    
    Parameters:
      d    : distance (can be numpy array)
      d0   : influence radius (no effect for d > d0)
      U_max: maximum potential (the spike value)
    
    Returns:
      U    : potential value(s)
    """
    d_thresh = saturation_frac * d0  # below this, the potential is saturated
    U = np.zeros_like(d)
    # Where the distance is very small, set the potential to the maximum:
    mask_inside = d < d_thresh
    U[mask_inside] = U_max
    # Where the distance is between d_thresh and d0, decay linearly:
    mask_linear = (d >= d_thresh) & (d <= d0)
    U[mask_linear] = U_max * (d0 - d[mask_linear]) / (d0 - d_thresh)
    # d > d0: contribution remains 0.
    return U

# --------------------------
# Compute Repulsive Contributions
# --------------------------
# Initialize the repulsive potential field
U_rep_total = np.zeros_like(X)

# For each obstacle
for (ox, oy) in obstacles:
    d = np.sqrt((X - ox)**2 + (Y - oy)**2)
    U_rep_total += normalized_potential(d, d0_rep, U_rep_max)

# # For walls, treat them as obstacles:
# # Left wall (x = 0) → distance is X
# U_rep_total += normalized_potential(X, d0_rep, U_rep_max)
# # Right wall (x = width) → distance is (width - X)
# U_rep_total += normalized_potential(width - X, d0_rep, U_rep_max)
# # Top wall (y = 0) → distance is Y
# U_rep_total += normalized_potential(Y, d0_rep, U_rep_max)
# # Bottom wall (y = height) → distance is (height - Y)
# U_rep_total += normalized_potential(height - Y, d0_rep, U_rep_max)

# --------------------------
# Compute Attractive Contributions
# --------------------------
# Initialize the attractive potential field
U_att_total = np.zeros_like(X)

for (tx, ty) in targets:
    d = np.sqrt((X - tx)**2 + (Y - ty)**2)
    # Compute the attractive potential contribution.
    # (Here we use the same kind of saturating function.
    #  You might want to flip the sign later to indicate an attractive well.)
    U_att_total += normalized_potential(d, d0_att, U_att_max)

# --------------------------
# Total Potential Field
# --------------------------
# Here, we treat repulsion as a barrier (positive) and attraction as a well (negative).
U_total = U_rep_total - U_att_total

# --------------------------
# Plotting the results
# --------------------------
fig = plt.figure(figsize=(14, 6))

# 3D Surface Plot
ax1 = fig.add_subplot(1, 2, 1, projection='3d')
surf = ax1.plot_surface(X, Y, U_total, cmap='viridis', edgecolor='none')
ax1.set_title('3D Potential Field')
ax1.set_xlabel('X')
ax1.set_ylabel('Y')
ax1.set_zlabel('Potential')
fig.colorbar(surf, ax=ax1, shrink=0.5, aspect=5)

# Contour Plot
ax2 = fig.add_subplot(1, 2, 2)
contour = ax2.contourf(X, Y, U_total, levels=50, cmap='viridis')
ax2.set_title('Potential Contour Plot')
ax2.set_xlabel('X')
ax2.set_ylabel('Y')
fig.colorbar(contour, ax=ax2)

plt.tight_layout()
plt.show()
