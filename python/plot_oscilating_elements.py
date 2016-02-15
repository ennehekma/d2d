'''
Copyright (c) 2014-2015, Kartik Kumar (me@kartikkumar.com)
Copyright (c) 2016, Enne Hekma (ennehekma@gmail.com)
All rights reserved.
'''

# Set up modules and packages
# Plotting
import matplotlib
import matplotlib as mpl
matplotlib.use('TkAgg')
import matplotlib.pyplot as plt
from matplotlib import rcParams
from matplotlib import cm
from mpl_toolkits.mplot3d import Axes3D
from mpl_toolkits.mplot3d import axes3d

# matplotlib.rcParams['text.usetex'] = True
# matplotlib.rcParams['text.latex.unicode'] = True

# I/O
import commentjson
import json
from pprint import pprint

# rcParams[axes.formatter.useoffset]=False
# rcParams["axes3d.formatter.useoffset"] = False
# print pcParams.keys()
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
path_elements = pd.read_csv(input_path_prefix + config["path_elements"])

print "Input data files successfully read!"

print "Figures being generated ..."

path_elements['jd'][0:-1] = (path_elements['jd'][0:-1]-path_elements['jd'][0:-1].min())*100000

fmt=matplotlib.ticker.ScalarFormatter(useOffset=False)

# Generate semi-major axis plot
ax = plt.subplot(111)
ax.yaxis.set_major_formatter(fmt)
plt.plot(path_elements['jd'][0:-1],path_elements['a'][0:-1],linewidth=2,label='Atom')
plt.plot(path_elements['jd'][0:-1],path_elements['La'][0:-1],label='Lambert',linewidth=2)
plt.xlabel('Time since beginning of the transfer [s]')
plt.ylabel('Semi-major axis [km]')
plt.title('Osculating element of transfer')
plt.legend(loc='best',prop={'size':9})
plt.grid()
plt.savefig(output_path_prefix + "sma_" + config["filename"], dpi=config["figure_dpi"])
plt.clf()

# Generate eccentricity plot
plt.plot(path_elements['jd'][0:-1],path_elements['e'][0:-1],linewidth=2,label='Atom')
plt.plot(path_elements['jd'][0:-1],path_elements['Le'][0:-1],label='Lambert',linewidth=2)
plt.xlabel('Time since beginning of the transfer [s]')
plt.ylabel('Eccentricity [-]')
plt.title('Osculating element of transfer')
plt.legend(loc='best',prop={'size':9})
plt.grid()
plt.savefig(output_path_prefix + "e_" + config["filename"], dpi=config["figure_dpi"])
plt.clf()

# # Generate inclination plot
plt.plot(path_elements['jd'][0:-1],path_elements['i'][0:-1],linewidth=2,label='Atom')
plt.plot(path_elements['jd'][0:-1],path_elements['Li'][0:-1],label='Lambert',linewidth=2)
plt.xlabel('Time since beginning of the transfer [s]')
plt.ylabel('Inclination [deg]')
plt.title('Osculating element of transfer')
plt.legend(loc='best',prop={'size':9})
plt.grid()
plt.savefig(output_path_prefix + "i_" + config["filename"], dpi=config["figure_dpi"])
plt.clf()

# # Generate argument of periapsis plot
plt.plot(path_elements['jd'][0:-1],path_elements['aop'][0:-1],label='Osculating element',linewidth=2)
plt.plot(path_elements['jd'][0:-1],path_elements['aop_dot_j2'][0:-1],label='J2 secular AoP change',linewidth=2,color='r')
# plt.plot(path_elements['jd'][0:-1],path_elements['aop_dot_moon'][0:-1],label='Moon 3rd body secular AoP change')
# plt.plot(path_elements['jd'][0:-1],path_elements['aop_dot_sun'][0:-1],label='Sun 3rd body secular AoP change')
# plt.plot(path_elements['jd'][0:-1],path_elements['aop_dot_total'][0:-1],label='Cumulated secular AoP change')
plt.plot(path_elements['jd'][0:-1],path_elements['aop_dot_3b'][0:-1],label='Third body (Sun and Moon) secular AoP change',linewidth=2,color='c')
plt.plot(path_elements['jd'][0:-1],path_elements['Laop'][0:-1],label='Lambert',linewidth=2,color='g')
plt.xlabel('Time since beginning of the transfer [s]')
plt.ylabel('Argument of periapsis [deg]')
plt.title('Osculating element of transfer')
plt.legend(loc='best',prop={'size':9})
plt.grid()
plt.savefig(output_path_prefix + "aop_" + config["filename"], dpi=config["figure_dpi"])
plt.clf()

# Generate RAAN plot
plt.plot(path_elements['jd'][0:-1],path_elements['raan'][0:-1],label='Osculating element',linewidth=2)
plt.plot(path_elements['jd'][0:-1],path_elements['raan_dot_j2'][0:-1],label='J2 secular RAAN change',linewidth=2,color='r')
# plt.plot(path_elements['jd'][0:-1],path_elements['raan_dot_moon'][0:-1],label='Moon 3rd body secular RAAN change')
# plt.plot(path_elements['jd'][0:-1],path_elements['raan_dot_sun'][0:-1],label='Sun 3rd body secular RAAN change')
# plt.plot(path_elements['jd'][0:-1],path_elements['raan_dot_total'][0:-1],label='Cumulated secular RAAN change')
plt.plot(path_elements['jd'][0:-1],path_elements['raan_dot_3b'][0:-1],label='Third body (Sun and Moon) secular RAAN change',linewidth=2,color='c')
plt.plot(path_elements['jd'][0:-1],path_elements['Lraan'][0:-1],label='Lambert',linewidth=2,color='g')
plt.xlabel('Time since beginning of the transfer [s]')
plt.ylabel('Right ascention of the ascending node [deg]')
plt.title('Osculating element of transfer')
plt.legend(loc='best',prop={'size':9})
plt.grid()
plt.savefig(output_path_prefix + "raan_" + config["filename"], dpi=config["figure_dpi"])
plt.clf()

# Generate true anomaly plot
plt.plot(path_elements['jd'][0:-1],path_elements['TA'][0:-1],linewidth=2,label='Atom')
plt.plot(path_elements['jd'][0:-1],path_elements['LTA'][0:-1],label='Lambert',linewidth=2)
plt.xlabel('Time since beginning of the transfer [s]')
plt.ylabel('True anomaly [deg]')
plt.title('Osculating element of transfer')
plt.legend(loc='best',prop={'size':9})
plt.grid()
plt.savefig(output_path_prefix + "TA_" + config["filename"], dpi=config["figure_dpi"])
plt.clf()



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