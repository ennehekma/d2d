'''
Copyright (c) 2014-2016 Kartik Kumar, Dinamica Srl (me@kartikkumar.com)
Copyright (c) 2016, Enne Hekma, Delft University of Technology (ennehekma@gmail.com)
Distributed under the MIT License.
See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
'''

# Set up modules and packages
# Plotting
import matplotlib
matplotlib.use('TkAgg')
import matplotlib.pyplot as plt
from matplotlib import rcParams
from matplotlib import cm
from mpl_toolkits.mplot3d import Axes3D
from mpl_toolkits.mplot3d import axes3d

# I/O
import commentjson
import json
from pprint import pprint

# Numerical
import numpy as np
import pandas as pd

# System
import sys
import time

print ""
print "------------------------------------------------------------------"
print "                               D2D                                "
print "                              0.0.2                               "
print "         Copyright (c) 2015, K. Kumar (me@kartikkumar.com)        "
print "------------------------------------------------------------------"
print ""

# Start timer.
start_time = time.time( )

print ""
print "******************************************************************"
print "                          Input parameters                        "
print "******************************************************************"
print ""

# Parse JSON configuration file
# Raise exception if wrong number of inputs are provided to script
if len(sys.argv) != 2:
    raise Exception("Only provide a JSON config file as input!")

json_data = open(sys.argv[1])
config = commentjson.load(json_data)
json_data.close()
pprint(config)

print ""
print "******************************************************************"
print "                            Operations                            "
print "******************************************************************"
print ""

print "Input data files being read ..."

input_path_prefix = config["input_directory"] + "/"
output_path_prefix = config["output_directory"] + str(config["tag"]) + "_"

# Read and store data files.
metadata = pd.read_csv(input_path_prefix + config["metadata"], header=None)
metadata_table = []
metadata_table.append(["Departure ID",int(metadata[1][0])," "])
metadata_table.append(["Arrival ID",int(metadata[1][1])," "])
metadata_table.append(["Departure epoch","{:,g}".format(float(metadata[1][2])),"JD"])
metadata_table.append(["Time-of-flight","{:,g}".format(float(metadata[1][3])),"s"])
metadata_table.append(["Prograde?",metadata[1][4].strip()," "])
metadata_table.append(["Revolutions",int(metadata[1][5])," "])
metadata_table.append([r"Transfer $\Delta V$","{:,g}".format(float(metadata[1][6])),"km/s"])

departure_orbit = pd.read_csv(input_path_prefix + config["departure_orbit"])
arrival_orbit = pd.read_csv(input_path_prefix + config["arrival_orbit"])
transfer_orbit = pd.read_csv(input_path_prefix + config["transfer_orbit"])

departure_path = pd.read_csv(input_path_prefix + config["departure_path"])
arrival_path = pd.read_csv(input_path_prefix + config["arrival_path"])
transfer_path = pd.read_csv(input_path_prefix + config["transfer_path"])

print "Input data files successfully read!"

print "figures being generated ..."

# Generate 3D figure if requested.
if config["show_3D_figure"]:
    fig = plt.figure()
    fig.canvas.set_window_title('3D')
    ax = fig.gca(projection='3d')
    ax.set_xlabel('x [km]')
    ax.set_ylabel('y [km]')
    ax.set_zlabel('z [km]')
    ax.ticklabel_format(style='sci', axis='both', scilimits=(0,0))

    # Plot sphere for the Earth
    radius_Earth = 6371.0 # km
    u = np.linspace(0, 2 * np.pi, 100)
    v = np.linspace(0, np.pi, 100)
    x = radius_Earth * np.outer(np.cos(u), np.sin(v))
    y = radius_Earth * np.outer(np.sin(u), np.sin(v))
    z = radius_Earth * np.outer(np.ones(np.size(u)), np.cos(v))
    ax.plot_surface(x, y, z,  rstride=4, cstride=4, color="lightskyblue",edgecolors="royalblue")

    # Plot departure and arrival orbits
    ax.plot3D(departure_orbit['x'],departure_orbit['y'],departure_orbit['z'],'g')
    ax.plot3D(arrival_orbit['x'],arrival_orbit['y'],arrival_orbit['z'],'r')

    # Plot transfer trajectory
    ax.plot3D(transfer_path['x'],transfer_path['y'],transfer_path['z'],'k')
    ax.scatter(transfer_path['x'][0],transfer_path['y'][0],transfer_path['z'][0], \
               s=100,marker='o',c='g')
    ax.scatter(transfer_path['x'][transfer_path.index[-1]], \
               transfer_path['y'][transfer_path.index[-1]], \
               transfer_path['z'][transfer_path.index[-1]],s=100,marker='o',c='r')

    # Create cubic bounding box to simulate equal aspect ratio
    X = departure_orbit['x']
    Y = departure_orbit['y']
    Z = departure_orbit['z']
    max_range = np.array([X.max()-X.min(), Y.max()-Y.min(), Z.max()-Z.min()]).max() / 2.0
    mean_x = X.mean()
    mean_y = Y.mean()
    mean_z = Z.mean()
    ax.set_xlim(mean_x - max_range, mean_x + max_range)
    ax.set_ylim(mean_y - max_range, mean_y + max_range)
    ax.set_zlim(mean_z - max_range, mean_z + max_range)
    print "Rotate 3D plot untill you like it and close the window to confirm the 3D plot to be added."
    
    projections = []
    
    def on_click(event):
        azim, elev = ax.azim, ax.elev
        projections.append((azim, elev))
        # print(azim, elev)
    cid = fig.canvas.mpl_connect('button_release_event', on_click)
    
    plt.grid()
    plt.show()
    # print projections[-1][0]
    a = int(projections[-1][0])
    e = int(projections[-1][1])
    # Request input for 3D figure camera input
    # a = raw_input("Please enter 3D figure camera input azimuth: ")
    # e = raw_input("Please enter 3D figure camera input elevation: ")
    print "3D plot will be generated and put in subplot with azimuth of:",a,"and an elevation of",e

# Generate figure with 2D views
fig = plt.figure()
ax1 = fig.add_subplot(2, 2, 1)
ax2 = fig.add_subplot(2, 2, 2)
ax3 = fig.add_subplot(2, 2, 3)
ax4 = fig.add_subplot(2, 2, 4, projection='3d')

# Plot X-Y projection
ax1.plot(departure_orbit['x'],departure_orbit['y'],color='c',linewidth=1.5)
ax1.plot(arrival_orbit['x'],arrival_orbit['y'],color='r',linewidth=1.5)
ax1.plot(transfer_path['x'],transfer_path['y'],color='k',linewidth=1.5)
ax1.scatter(transfer_path['x'][0],transfer_path['y'][0],s=100,marker='o',color='c')
ax1.scatter(transfer_path['x'][transfer_path.index[-1]], \
            transfer_path['y'][transfer_path.index[-1]],s=100,marker='o',color='r')
ax1.arrow(departure_orbit['x'][0],departure_orbit['y'][0],171.5518970411*10,326.741914447837*10,linewidth=2, head_width=300, head_length=500, fc='b', ec='b')
ax1.arrow(departure_orbit['x'][0],departure_orbit['y'][0],133.7789900016*10,282.4239691231*10,linewidth=2, head_width=300, head_length=500, fc='g', ec='g')

ax1.arrow(arrival_orbit['x'][0],arrival_orbit['y'][0],84.1820245801 *10,55.5782226525*10,linewidth=2, head_width=300, head_length=500, fc='b', ec='b',label='Atom')
ax1.arrow(arrival_orbit['x'][0],arrival_orbit['y'][0],76.3979223991*10,42.1782365524*10,linewidth=2, head_width=300, head_length=500, fc='g', ec='g', label='Lambert')
ax1.set_xlabel('x [km]')
ax1.set_ylabel('y [km]')
ax1.ticklabel_format(style='sci', axis='both', scilimits=(0,0))
ax1.grid()

# Plot X-Z projection
ax2.plot(departure_orbit['x'],departure_orbit['z'],color='c',linewidth=1.5)
ax2.plot(arrival_orbit['x'],arrival_orbit['z'],color='r',linewidth=1.5)
ax2.plot(transfer_path['x'],transfer_path['z'],color='k',linewidth=1.5)
ax2.scatter(transfer_path['x'][0],transfer_path['z'][0],s=100,marker='o',color='c')
ax2.scatter(transfer_path['x'][transfer_path.index[-1]], \
            transfer_path['z'][transfer_path.index[-1]],s=100,marker='o',color='r')

ax2.arrow(departure_orbit['x'][0],departure_orbit['z'][0],171.5518970411*10,208.2453118797*10,linewidth=2, head_width=300, head_length=500, fc='b', ec='b')
ax2.arrow(departure_orbit['x'][0],departure_orbit['z'][0],133.7789900016*10,126.2019499345*10,linewidth=2, head_width=300, head_length=500, fc='g', ec='g')

ax2.arrow(arrival_orbit['x'][0],arrival_orbit['z'][0],84.1820245801*10,-30.3099452923*10,linewidth=2, head_width=300, head_length=500, fc='b', ec='b',label='Atom')
ax2.arrow(arrival_orbit['x'][0],arrival_orbit['z'][0],76.3979223991*10,-65.9155544783*10,linewidth=2, head_width=300, head_length=500, fc='g', ec='g', label='Lambert')
ax2.set_xlabel('x [km]')
ax2.set_ylabel('z [km]')
# ax2.set_xlim()
ax2.set_ylim(-4e3,6.3e3)
ax2.ticklabel_format(style='sci', axis='both', scilimits=(0,0))
ax2.grid()

# Plot Y-Z projection
ax3.plot(departure_orbit['y'],departure_orbit['z'],color='c',linewidth=1.5)
ax3.plot(arrival_orbit['y'],arrival_orbit['z'],color='r',linewidth=1.5)
ax3.plot(transfer_path['y'],transfer_path['z'],color='k',linewidth=1.5)
ax3.scatter(transfer_path['y'][0],transfer_path['z'][0],s=100,marker='o',color='c')
ax3.scatter(transfer_path['y'][transfer_path.index[-1]], \
            transfer_path['z'][transfer_path.index[-1]],s=100,marker='o',color='r')
ax3.arrow(departure_orbit['y'][0],departure_orbit['z'][0],326.7419144478*10,208.2453118797*10,linewidth=2, head_width=300, head_length=500, fc='b', ec='b')
ax3.arrow(departure_orbit['y'][0],departure_orbit['z'][0],282.4239691231*10,126.2019499345*10,linewidth=2, head_width=300, head_length=500, fc='g', ec='g')

ax3.arrow(arrival_orbit['y'][0],arrival_orbit['z'][0],55.5782226525*10,-30.3099452923*10,linewidth=2, head_width=300, head_length=500, fc='b', ec='b',label='Atom')
ax3.arrow(arrival_orbit['y'][0],arrival_orbit['z'][0],42.1782365524*10,-65.9155544783*10,linewidth=2, head_width=300, head_length=500, fc='g', ec='g', label='Lambert')
ax3.set_xlabel('y [km]')
ax3.set_ylabel('z [km]')
ax3.set_ylim(-4e3,6e3)
ax3.ticklabel_format(style='sci', axis='both', scilimits=(0,0))
ax3.grid()


if not config["show_3D_figure"]:
    # Plot metadata table
    ax4.axis('off')
    the_table = ax4.table(cellText=metadata_table,colLabels=None,cellLoc='center',loc='center')
    table_props = the_table.properties()
    table_cells = table_props['child_artists']
    for cell in table_cells: cell.set_height(0.15)
    cell_dict = the_table.get_celld()
    for row in xrange(0,7): cell_dict[(row,2)].set_width(0.1)

if config["show_3D_figure"]:
    # Plot 3D projection
    font_size = 8
    ax4.set_xlabel('x [km]',fontsize=font_size)
    ax4.set_ylabel('y [km]',fontsize=font_size)
    ax4.set_zlabel('z [km]',fontsize=font_size)
    ax4.set_xticks([-6000,0,6000])
    ax4.set_yticks([-6000,0,6000])
    ax4.set_zticks([-6000,0,6000])
    ax4.ticklabel_format(useOffset=False)
    ax4.tick_params(axis='x', labelsize=font_size)
    ax4.tick_params(axis='y', labelsize=font_size)
    ax4.tick_params(axis='z', labelsize=font_size)
    # ax4.set_xticklabels('0','6000')
    # ax4.ticklabel_format(style='sci', axis='both', scilimits=(0,0))

    # Plot sphere for the Earth
    radius_Earth = 6371.0 # km
    u = np.linspace(0, 2 * np.pi, 100)
    v = np.linspace(0, np.pi, 100)
    x = radius_Earth * np.outer(np.cos(u), np.sin(v))
    y = radius_Earth * np.outer(np.sin(u), np.sin(v))
    z = radius_Earth * np.outer(np.ones(np.size(u)), np.cos(v))
    ax4.plot_surface(x, y, z,  rstride=4, cstride=4, alpha=1, color="lightskyblue",edgecolors="royalblue")

    # Plot departure and arrival orbits
    ax4.plot3D(departure_orbit['x'],departure_orbit['y'],departure_orbit['z'],'g')
    ax4.plot3D(arrival_orbit['x'],arrival_orbit['y'],arrival_orbit['z'],'r')

    # Plot transfer trajectory
    ax4.plot3D(transfer_path['x'],transfer_path['y'],transfer_path['z'],'k')
    ax4.scatter(transfer_path['x'][0],transfer_path['y'][0],transfer_path['z'][0], \
               s=100,marker='o',c='g')
    ax4.scatter(transfer_path['x'][transfer_path.index[-1]], \
               transfer_path['y'][transfer_path.index[-1]], \
               transfer_path['z'][transfer_path.index[-1]],s=100,marker='o',c='r')

    # Create cubic bounding box to simulate equal aspect ratio
    X = departure_orbit['x']
    Y = departure_orbit['y']
    Z = departure_orbit['z']
    max_range = np.array([X.max()-X.min(), Y.max()-Y.min(), Z.max()-Z.min()]).max() / 2.0
    mean_x = X.mean()
    mean_y = Y.mean()
    mean_z = Z.mean()
    ax4.set_xlim(mean_x - max_range, mean_x + max_range)
    ax4.set_ylim(mean_y - max_range, mean_y + max_range)
    ax4.set_zlim(mean_z - max_range, mean_z + max_range)
    ax4.view_init(azim=int(a), elev=int(e))
    # ax4.view_init(azim = 0 ,elev = 0)

# Save figure
# ax1.legend(["Atom44","Atomdsa","Atomfasd","Atom","Atom2","Atom3","Atom4", 'b'])
# ax1.legend(['Atom','Lambert'])
plt.tight_layout()
plt.savefig(output_path_prefix + "_velocity_vector" + config["2D_figure"], dpi=config["figure_dpi"])
plt.show()


print "Figures generated successfully!"
print ""

# Stop timer
end_time = time.time( )

# Print elapsed time
print "Script time: " + str("{:,g}".format(end_time - start_time)) + "s"

print ""
print "------------------------------------------------------------------"
print "                         Exited successfully!                     "
print "------------------------------------------------------------------"
print ""