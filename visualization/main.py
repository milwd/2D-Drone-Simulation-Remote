#!/usr/bin/env python3
import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D

width = 30      # domain width
height = 20     # domain height
n_points = 100  # resolution for plotting

x = np.linspace(0, width, n_points)
y = np.linspace(0, height, n_points)
X, Y = np.meshgrid(x, y)

obstacles = [(5, 5), (15, 15), (10, 10)]
targets   = [(5, 15), (15, 5)]

d0_rep = 4.0   # repulsive influence radius
d0_att = 4.0   # attractive influence radius

U_rep_max = 200.0  # maximum repulsive potential (barrier)
U_att_max = 200.0  # maximum attractive potential (well); note: we subtract it later

saturation_frac = 0.1  # i.e. if d < 10% of d0, use the maximum

def normalized_potential(d, d0, U_max):
    d_thresh = saturation_frac * d0  
    U = np.zeros_like(d)
    mask_inside = d < d_thresh
    U[mask_inside] = U_max
    mask_linear = (d >= d_thresh) & (d <= d0)
    U[mask_linear] = U_max * (d0 - d[mask_linear]) / (d0 - d_thresh)
    return U

U_rep_total = np.zeros_like(X)

for (ox, oy) in obstacles:
    d = np.sqrt((X - ox)**2 + (Y - oy)**2)
    U_rep_total += normalized_potential(d, d0_rep, U_rep_max)

U_rep_total += 0.3 * normalized_potential(X, d0_rep, U_rep_max)
U_rep_total += 0.3 * normalized_potential(width - X, d0_rep, U_rep_max)
U_rep_total += 0.3 * normalized_potential(Y, d0_rep, U_rep_max)
U_rep_total += 0.3 * normalized_potential(height - Y, d0_rep, U_rep_max)

U_att_total = np.zeros_like(X)

for (tx, ty) in targets:
    d = np.sqrt((X - tx)**2 + (Y - ty)**2)
    # Compute the attractive potential contribution.
    # (Here we use the same kind of saturating function.
    #  You might want to flip the sign later to indicate an attractive well.)
    U_att_total += normalized_potential(d, d0_att, U_att_max)

U_total = U_rep_total - U_att_total

fig = plt.figure(figsize=(14, 6))
ax1 = fig.add_subplot(1, 2, 1, projection='3d')
surf = ax1.plot_surface(X, Y, U_total, cmap='viridis', edgecolor='none')
ax1.set_title('3D Potential Field')
ax1.set_xlabel('X')
ax1.set_ylabel('Y')
ax1.set_zlabel('Potential')
fig.colorbar(surf, ax=ax1, shrink=0.5, aspect=5)
ax2 = fig.add_subplot(1, 2, 2)
contour = ax2.contourf(X, Y, U_total, levels=50, cmap='viridis')
ax2.set_title('Potential Contour Plot')
ax2.set_xlabel('X')
ax2.set_ylabel('Y')
fig.colorbar(contour, ax=ax2)
plt.tight_layout()
plt.show()
