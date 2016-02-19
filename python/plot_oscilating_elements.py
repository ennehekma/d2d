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
oscilating_elements_half = pd.read_csv(input_path_prefix + config["oscilating_elements_half"])
oscilating_elements = pd.read_csv(input_path_prefix + config["oscilating_elements"])
oscilating_elements_twice = pd.read_csv(input_path_prefix + config["oscilating_elements_twice"])
oscilating_elements_10 = pd.read_csv(input_path_prefix + config["oscilating_elements_10"])
# oscilating_elements_100 = pd.read_csv(input_path_prefix + config["oscilating_elements_100"])

print "Input data files successfully read!"

print "Figures being generated ..."

oscilating_elements['jd'][0:-1] = (oscilating_elements['jd'][0:-1]-oscilating_elements['jd'][0:-1].min())*100000

formatter = matplotlib.ticker.ScalarFormatter(useOffset=False)

# Generate semi-major axis plot
fig=plt.figure()
ax1 = fig.add_subplot(111)
plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements_twice['a'][0:-1]-oscilating_elements_10['a'][0:-1],linewidth=1,label='BStar twice mean - BStar zero')
# plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements_half['a'][0:-1],linewidth=1,label='BStar half mean')
# plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements['a'][0:-1],linewidth=1,label='BStar mean')
# plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements_twice['a'][0:-1],linewidth=1,label='BStar twice mean')
# plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements_10['a'][0:-1],linewidth=1,label='BStar 10x mean')
# plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements_100['a'][0:-1],linewidth=1,label='BStar 100x mean')
# plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements['La'][0:-1],label='Lambert',linewidth=1)
ax1.xaxis.set_major_formatter(formatter)
ax1.yaxis.set_major_formatter(formatter)
plt.xlabel('Time since beginning of the transfer [s]')
plt.ylabel('Semi-major axis [km]')
plt.title('Osculating element of transfer')
plt.legend(loc='best',prop={'size':9})
plt.grid()
plt.savefig(output_path_prefix + "sma_" + config["filename"], dpi=config["figure_dpi"])
plt.clf()

# Generate eccentricity plot
fig=plt.figure()
ax1 = fig.add_subplot(111)
plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements_twice['e'][0:-1]-oscilating_elements_10['e'][0:-1],linewidth=1,label='BStar twice mean - BStar zero')
# plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements_half['e'][0:-1],linewidth=1,label='BStar half mean')
# plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements['e'][0:-1],linewidth=1,label='BStar mean')
# plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements_twice['e'][0:-1],linewidth=1,label='BStar twice mean')
# plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements_10['e'][0:-1],linewidth=1,label='BStar 10x mean')
# plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements_100['e'][0:-1],linewidth=1,label='BStar 100x mean')
# plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements['Le'][0:-1],label='Lambert',linewidth=1)
ax1.xaxis.set_major_formatter(formatter)
ax1.yaxis.set_major_formatter(formatter)
plt.xlabel('Time since beginning of the transfer [s]')
plt.ylabel('Eccentricity [-]')
plt.title('Osculating element of transfer')
plt.legend(loc='best',prop={'size':9})
plt.grid()
plt.savefig(output_path_prefix + "e_" + config["filename"], dpi=config["figure_dpi"])
plt.clf()

# # Generate inclination plot
fig=plt.figure()
ax1 = fig.add_subplot(111)
plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements_twice['i'][0:-1]-oscilating_elements_10['i'][0:-1],linewidth=1,label='BStar twice mean - BStar zero')
# plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements_half['i'][0:-1],linewidth=1,label='BStar half mean')
# plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements['i'][0:-1],linewidth=1,label='BStar mean')
# plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements_twice['i'][0:-1],linewidth=1,label='BStar twice mean')
# plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements_10['i'][0:-1],linewidth=1,label='BStar 10x mean')
# plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements_100['i'][0:-1],linewidth=1,label='BStar 100x mean')
# plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements['Li'][0:-1],label='Lambert',linewidth=1)
ax1.xaxis.set_major_formatter(formatter)
ax1.yaxis.set_major_formatter(formatter)
plt.xlabel('Time since beginning of the transfer [s]')
plt.ylabel('Inclination [deg]')
plt.title('Osculating element of transfer')
plt.legend(loc='best',prop={'size':9})
plt.grid()
plt.savefig(output_path_prefix + "i_" + config["filename"], dpi=config["figure_dpi"])
plt.clf()

# Generate argument of periapsis plot
fig=plt.figure()
ax1 = fig.add_subplot(111)
plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements_twice['aop'][0:-1]-oscilating_elements_10['aop'][0:-1],linewidth=1,label='BStar twice mean - BStar zero')
# plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements_half['aop'][0:-1],label='BStar half mean',linewidth=1)
# plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements['aop'][0:-1],label='BStar mean',linewidth=1)
# plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements_twice['aop'][0:-1],label='BStar twice mean',linewidth=1)
# plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements_10['aop'][0:-1],label='BStar 10x mean',linewidth=1)
# plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements_100['aop'][0:-1],label='BStar 100x mean',linewidth=1)
# plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements['aop_dot_j2'][0:-1],label='J2 secular AoP change',linewidth=1,color='r')
# plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements['aop_dot_moon'][0:-1],label='Moon 3rd body secular AoP change')
# plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements['aop_dot_sun'][0:-1],label='Sun 3rd body secular AoP change')
# plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements['aop_dot_total'][0:-1],label='Cumulated secular AoP change')
# plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements['aop_dot_3b'][0:-1],label='Third body (Sun and Moon) secular AoP change',linewidth=1,color='c')
# plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements['Laop'][0:-1],label='Lambert',linewidth=1,color='g')
ax1.xaxis.set_major_formatter(formatter)
ax1.yaxis.set_major_formatter(formatter)
plt.xlabel('Time since beginning of the transfer [s]')
plt.ylabel('Argument of periapsis [deg]')
plt.title('Osculating element of transfer')
plt.legend(loc='best',prop={'size':9})
plt.grid()
plt.savefig(output_path_prefix + "aop_" + config["filename"], dpi=config["figure_dpi"])
plt.clf()

# Generate RAAN plot
fig=plt.figure()
ax1 = fig.add_subplot(111)
plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements_twice['raan'][0:-1]-oscilating_elements_10['raan'][0:-1],linewidth=1,label='BStar twice mean - BStar zero')
# plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements_half['raan'][0:-1],label='BStar half mean',linewidth=1)
# plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements['raan'][0:-1],label='BStar mean',linewidth=1)
# plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements_twice['raan'][0:-1],label='BStar twice mean',linewidth=1)
# plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements_10['raan'][0:-1],label='BStar 10x mean',linewidth=1)
# plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements_100['raan'][0:-1],label='BStar 100x mean',linewidth=1)
# plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements['raan_dot_j2'][0:-1],label='J2 secular RAAN change',linewidth=1,color='r')
# plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements['raan_dot_moon'][0:-1],label='Moon 3rd body secular RAAN change')
# plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements['raan_dot_sun'][0:-1],label='Sun 3rd body secular RAAN change')
# plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements['raan_dot_total'][0:-1],label='Cumulated secular RAAN change')
# plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements['raan_dot_3b'][0:-1],label='Third body (Sun and Moon) secular RAAN change',linewidth=1,color='c')
# plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements['Lraan'][0:-1],label='Lambert',linewidth=1,color='g')
ax1.xaxis.set_major_formatter(formatter)
ax1.yaxis.set_major_formatter(formatter)
plt.xlabel('Time since beginning of the transfer [s]')
plt.ylabel('Right ascention of the ascending node [deg]')
plt.title('Osculating element of transfer')
plt.legend(loc='best',prop={'size':9})
plt.grid()
plt.savefig(output_path_prefix + "raan_" + config["filename"], dpi=config["figure_dpi"])
plt.clf()

# Generate true anomaly plot
fig=plt.figure()
ax1 = fig.add_subplot(111)
plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements_twice['TA'][0:-1]-oscilating_elements_10['TA'][0:-1],linewidth=1,label='BStar twice mean - BStar zero')
# plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements_half['TA'][0:-1],linewidth=1,label='BStar half mean')
# plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements['TA'][0:-1],linewidth=1,label='BStar mean')
# plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements_twice['TA'][0:-1],linewidth=1,label='BStar twice mean')
# plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements_10['TA'][0:-1],linewidth=1,label='BStar 10x mean')
# plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements_100['TA'][0:-1],linewidth=1,label='BStar 100x mean')
# plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements['LTA'][0:-1],label='Lambert',linewidth=1)
ax1.xaxis.set_major_formatter(formatter)
ax1.yaxis.set_major_formatter(formatter)
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