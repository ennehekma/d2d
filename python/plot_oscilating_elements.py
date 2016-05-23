#!/usr/bin/python
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
oscilating_elements = pd.read_csv(input_path_prefix + config["oscilating_elements"])
oscilating_elements2 = pd.read_csv(input_path_prefix + config["oscilating_elements2"])

print "Input data files successfully read!"

print "Figures being generated ..."

oscilating_elements['jd'][0:-1] = (oscilating_elements['jd'][0:-1]-oscilating_elements['jd'][0:-1].min())*86400
oscilating_elements2['jd'][0:-1] = (oscilating_elements2['jd'][0:-1]-oscilating_elements2['jd'][0:-1].min())*86400

formatter = matplotlib.ticker.ScalarFormatter(useOffset=False)

# Generate semi-major axis plot
fig=plt.figure()
ax1 = fig.add_subplot(111)
plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements['a'][0:-1],'b',linewidth=2,label='Atom')
plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements['La'][0:-1],'g--',label='Lambert',linewidth=2)
plt.plot(oscilating_elements2['jd'][0:-1],oscilating_elements2['a'][0:-1],'b',linewidth=2,label='Atom')
plt.plot(oscilating_elements2['jd'][0:-1],oscilating_elements2['La'][0:-1],'g--',label='Lambert',linewidth=2)
ax1.xaxis.set_major_formatter(formatter)
ax1.yaxis.set_major_formatter(formatter)
plt.xlabel('Time since beginning of the transfer [s]',size='17')
plt.ylabel('Semi-major axis [km]',size='17')
# plt.title('Osculating element of transfer')
# plt.legend(loc='best',prop={'size':9})
plt.grid()
plt.savefig(output_path_prefix + "semiMajorAxis" + config["filename"], dpi=config["figure_dpi"])
plt.clf()

# Generate eccentricity plot
fig=plt.figure()
ax1 = fig.add_subplot(111)
plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements['e'][0:-1],'b',linewidth=2,label='Atom')
plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements['Le'][0:-1],'g--',label='Lambert',linewidth=2)
plt.plot(oscilating_elements2['jd'][0:-1],oscilating_elements2['e'][0:-1],'b',linewidth=2,label='Atom')
plt.plot(oscilating_elements2['jd'][0:-1],oscilating_elements2['Le'][0:-1],'g--',label='Lambert',linewidth=2)
ax1.xaxis.set_major_formatter(formatter)
ax1.yaxis.set_major_formatter(formatter)
plt.xlabel('Time since beginning of the transfer [s]',size='17')
plt.ylabel('Eccentricity [-]',size='17')
# plt.title('Osculating element of transfer')
# plt.legend(loc='best',prop={'size':9})
plt.grid()
plt.savefig(output_path_prefix + "eccentricity" + config["filename"], dpi=config["figure_dpi"])
plt.clf()

# # Generate inclination plot
fig=plt.figure()
ax1 = fig.add_subplot(111)
plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements['i'][0:-1],'b',linewidth=2,label='Atom')
plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements['Li'][0:-1],'g--',label='Lambert',linewidth=2)
plt.plot(oscilating_elements2['jd'][0:-1],oscilating_elements2['i'][0:-1],'b',linewidth=2,label='Atom')
plt.plot(oscilating_elements2['jd'][0:-1],oscilating_elements2['Li'][0:-1],'g--',label='Lambert',linewidth=2)
ax1.xaxis.set_major_formatter(formatter)
ax1.yaxis.set_major_formatter(formatter)
plt.xlabel('Time since beginning of the transfer [s]',size='17')
plt.ylabel('Inclination [deg]',size='17')
# if oscilating_elements['i'][0]==30.4817644032608:
# plt.axis([0,25000,30,30.6])
# plt.axis([0,16000,98.6,99.6])
# plt.title('Osculating element of transfer')
# plt.legend(loc='best',prop={'size':9})
plt.grid()
plt.savefig(output_path_prefix + "inclination" + config["filename"], dpi=config["figure_dpi"])
plt.clf()

# Generate argument of periapsis plot
fig=plt.figure()
ax1 = fig.add_subplot(111)
plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements['aop'][0:-1],'b',label='Osculating element',linewidth=2)
plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements['aop_dot_j2'][0:-1],'r--', dashes=(15,5),label='J2 secular AoP change',linewidth=2)
plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements['Laop'][0:-1],'g--',label='Lambert',linewidth=2,color='g')
plt.plot(oscilating_elements2['jd'][0:-1],oscilating_elements2['aop'][0:-1],'b',label='Osculating element',linewidth=2)
plt.plot(oscilating_elements2['jd'][0:-1],oscilating_elements2['aop_dot_j2'][0:-1],'r--', dashes=(15,5),label='J2 secular AoP change',linewidth=2)
plt.plot(oscilating_elements2['jd'][0:-1],oscilating_elements2['Laop'][0:-1],'g--',label='Lambert',linewidth=2,color='g')
# plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements['aop_dot_moon'][0:-1],label='Moon 3rd body secular AoP change')
# plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements['aop_dot_sun'][0:-1],label='Sun 3rd body secular AoP change')
# plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements['aop_dot_total'][0:-1],label='Cumulated secular AoP change')
# plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements['aop_dot_3b'][0:-1],label='Third body (Sun and Moon) secular AoP change',linewidth=2,color='c')
ax1.xaxis.set_major_formatter(formatter)
ax1.yaxis.set_major_formatter(formatter)
# plt.axhline(360, linestyle='-', color='k',linewidth=1) # thick horizontal line at zero
plt.xlabel('Time since beginning of the transfer [s]',size='17')
plt.ylabel('Argument of periapsis [deg]',size='17')
# plt.title('Osculating element of transfer')
# plt.legend(loc='best',prop={'size':9})
# labels = [item.get_text() for item in ax1.get_yticklabels()]
# labels[0] = '340'
# labels[1] = '345'
# labels[2] = '350'
# labels[3] = '355'
# labels[4] = '0'
# labels[5] = '5'

# ax1.set_yticklabels(labels)

plt.grid()
plt.savefig(output_path_prefix + "argumentOfPeriapsis" + config["filename"], dpi=config["figure_dpi"])
plt.clf()

# Generate RAAN plot
fig=plt.figure()
ax1 = fig.add_subplot(111)
plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements['raan'][0:-1],'b',label='Osculating element',linewidth=2)
plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements['raan_dot_j2'][0:-1],'r--', dashes=(15,5),label='J2 secular RAAN change',linewidth=2)
plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements['Lraan'][0:-1],'g--',label='Lambert',linewidth=2,color='g')
plt.plot(oscilating_elements2['jd'][0:-1],oscilating_elements2['raan'][0:-1],'b',label='Osculating element',linewidth=2)
plt.plot(oscilating_elements2['jd'][0:-1],oscilating_elements2['raan_dot_j2'][0:-1],'r--', dashes=(15,5),label='J2 secular RAAN change',linewidth=2)
plt.plot(oscilating_elements2['jd'][0:-1],oscilating_elements2['Lraan'][0:-1],'g--',label='Lambert',linewidth=2,color='g')
# plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements['raan_dot_moon'][0:-1],label='Moon 3rd body secular RAAN change')
# plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements['raan_dot_sun'][0:-1],label='Sun 3rd body secular RAAN change')
# plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements['raan_dot_total'][0:-1],label='Cumulated secular RAAN change')
# plt.plot(oscilating_elements['jd'][0:-1],oscilating_elements['raan_dot_3b'][0:-1],label='Third body (Sun and Moon) secular RAAN change',linewidth=2,color='c')
ax1.xaxis.set_major_formatter(formatter)
ax1.yaxis.set_major_formatter(formatter)
plt.axis([0,3000,106,108])
# plt.axis([0,16000,106.5,109])
plt.xlabel('Time since beginning of the transfer [s]',size='17')
plt.ylabel('Right ascention of the ascending node [deg]',size='17')
# plt.title('Osculating element of transfer')
# plt.legend(loc='best',prop={'size':9})
plt.grid()
plt.savefig(output_path_prefix + "rightAscensionOfAscendingNode" + config["filename"], dpi=config["figure_dpi"])
plt.clf()

# Generate true anomaly plot
fig=plt.figure()
ax = fig.add_subplot(111)
# ax.spines['top'].set_color('none')
# ax.spines['bottom'].set_color('none')
# ax.spines['left'].set_color('none')
# ax.spines['right'].set_color('none')
# ax.tick_params(labelcolor='w', top='off', bottom='off', left='off', right='off')

# ax2 = fig.add_subplot(212)
ax2 = fig.add_subplot(111)
ax2.plot(oscilating_elements2['jd'][0:-1],oscilating_elements2['TA'][0:-1],'b',linewidth=2,label='Atom')
ax2.plot(oscilating_elements2['jd'][0:-1],oscilating_elements2['LTA'][0:-1],'g--',label='Lambert',linewidth=2)
# ax.grid()
# ax1 = fig.add_subplot(211,sharex=ax2)
# ax1.plot(oscilating_elements['jd'][0:-1],oscilating_elements['TA'][0:-1],'b',linewidth=2,label='Atom')
# ax1.plot(oscilating_elements['jd'][0:-1],oscilating_elements['LTA'][0:-1],'g--',label='Lambert',linewidth=2)

ax1.xaxis.set_major_formatter(formatter)
ax1.yaxis.set_major_formatter(formatter)
ax.set_xlabel('Time since beginning of the transfer [s]',size='17')
ax.set_ylabel('True anomaly [deg]',size='17')
# plt.title('Osculating element of transfer')
# plt.legend(loc='best',prop={'size':9})
ax1.grid()
ax2.grid()
plt.savefig(output_path_prefix + "trueAnomaly" + config["filename"], dpi=config["figure_dpi"])
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
