'''
Copyright (c) 2014-2015, Kartik Kumar (me@kartikkumar.com)
Copyright (c) 2016, Enne Hekma (ennehekma@gmail.com)
All rights reserved.
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
residuals = pd.read_csv(input_path_prefix + config["residuals"], delim_whitespace=True)

print "Input data files successfully read!"

print "Figures being generated ..."

plt.plot(residuals['#'][0:-2].astype(float),residuals['f1'][0:-2].astype(float),label='X-coordinate')
plt.plot(residuals['#'][0:-2].astype(float),residuals['f2'][0:-2].astype(float),label='Y-coordinate')
plt.plot(residuals['#'][0:-2].astype(float),residuals['f3'][0:-2].astype(float),label='Z-coordinate')
plt.axhline(0, linestyle='-', color='k',linewidth=2) # thick horizontal line at zero

plt.xlabel('Iterations [-]')
plt.ylabel('Residual [-]')
plt.title('Residuals of ATOM solver for all axis')

plt.legend()
plt.grid()
# plt.show()

# Save figure
plt.tight_layout()
plt.savefig(output_path_prefix + config["2D_figure"], dpi=config["figure_dpi"])
# plt.show()


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