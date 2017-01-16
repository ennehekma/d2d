'''
Copyright (c) 2014-2016, Kartik Kumar (me@kartikkumar.com)
Copyright (c) 2016, Enne Hekma (ennehekma@gmail.com)
All rights reserved.
'''

# Set up modules and packages
# Plotting
import matplotlib
matplotlib.use('TkAgg')
import matplotlib.pyplot as plt
from matplotlib import gridspec
from mpl_toolkits.mplot3d import Axes3D
from mpl_toolkits.mplot3d import axes3d
from nlcmap import nlcmap
from matplotlib import rcParams
from mpl_toolkits.axes_grid1.inset_locator import inset_axes
import math

# I/O
import commentjson
import json
from pprint import pprint
import sqlite3

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
print "      Copyright (c) 2015-2016, K. Kumar (me@kartikkumar.com)      "
print "      Copyright (c) 2016, E.J. Hekma (ennehekma@gmail.com)        "
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

print "Fetching porkchop plot data from database ..."

# Connect to SQLite database.
try:
    database = sqlite3.connect(config['database'])

except sqlite3.Error, e:
    print "Error %s:" % e.args[0]
    sys.exit(1)

if config['objects']==[]:
	best = pd.read_sql("SELECT DISTINCT(departure_object_id), arrival_object_id, time_of_flight,  \
										departure_epoch, transfer_delta_v  						  \
	                    FROM lambert_scanner_results 											  \
	 					ORDER BY transfer_delta_v ASC 											  \
	 					LIMIT 10 ",																  \
	                    database )

	a = (best['departure_object_id'][0])
	b = (best['arrival_object_id'][0])
else:
	a = config['objects'][0]
	b = config['objects'][1]

print "Porkchop plot figure being generated for transfer between TLE objects", a, "and", b, "..."

raw_data = pd.read_sql_query("	SELECT 	time_of_flight,									  		  \
										departure_epoch,			  				  		 	  \
										transfer_delta_v, 								  		  \
										departure_position_x	  						  		  \
										FROM 	lambert_scanner_results \
										WHERE departure_object_id == \
										"  + str(a) + "\
										 and arrival_object_id ==  \
										 " + str(b) + ";",						  \
										database)

raw_data.columns = [ 	'time_of_flight',
						'departure_epoch',
						'transfer_delta_v',
						'iteration' ]

first_departure_epoch = raw_data['departure_epoch'][0]
raw_data['departure_epoch'] = raw_data['departure_epoch'] - first_departure_epoch
raw_data['time_of_flight'] = raw_data['time_of_flight']/86400
number_of_iterations = int(raw_data['iteration'][raw_data['iteration'].argmax()])

# Add cutoff dV if user input requires to do so
if config['transfer_deltaV_cutoff'] != 0:
	cutoff = config['transfer_deltaV_cutoff']
	for i in xrange(0,len(raw_data)):
		if raw_data['transfer_delta_v'][i] > cutoff:
			raw_data['transfer_delta_v'][i] = cutoff

max_time_of_flight = raw_data['time_of_flight'][raw_data['time_of_flight'].argmax()]
max_departure_epoch = raw_data['departure_epoch'][raw_data['departure_epoch'].argmax()]

timestep = raw_data['time_of_flight'][1]-raw_data['time_of_flight'][0]
timestep = timestep*2*86400
for x in xrange(0,number_of_iterations+1):
	print "Plotting scatterplot ",x+1
	timestep = timestep/2
	print "Timestep :",timestep

	bestuptillnow = raw_data.loc[raw_data['iteration']<=x]

	tempstring=raw_data.loc[raw_data['iteration']==x]
	tempstringold=raw_data.loc[raw_data['iteration']==x-1]
	tempstringoldold=raw_data.loc[raw_data['iteration']==x-2]


	best_dv = str(round((raw_data['transfer_delta_v'][raw_data['transfer_delta_v'].argmin()])*1000,1))
	x_best_dv = raw_data['departure_epoch'][raw_data['transfer_delta_v'].argmin()]
	y_best_dv = raw_data['time_of_flight'][raw_data['transfer_delta_v'].argmin()]

	temp_best = str(round((bestuptillnow['transfer_delta_v'][bestuptillnow['transfer_delta_v'].argmin()])*1000,1))
	x_temp_best = bestuptillnow['departure_epoch'][bestuptillnow['transfer_delta_v'].argmin()]
	y_temp_best = bestuptillnow['time_of_flight'][bestuptillnow['transfer_delta_v'].argmin()]


	# Plot porkchop plot
	cmap = plt.get_cmap(config['colormap'])
	fig=plt.figure()
	ax1 = fig.add_subplot(111)

	data = ax1.scatter(	tempstring['departure_epoch'],
						tempstring['time_of_flight'],
						c=tempstring['transfer_delta_v'],
						cmap=cmap,
						s=2,
						lw=0)
	ax1.scatter(		tempstringold['departure_epoch'],
						tempstringold['time_of_flight'],
						c=tempstringold['transfer_delta_v'],
						cmap=cmap,
						s=0.5,
						lw=0)
	ax1.scatter( 		tempstringoldold['departure_epoch'],
						tempstringoldold['time_of_flight'],
						c=tempstringoldold['transfer_delta_v'],
						cmap=cmap,
						s=0.1,
						lw=0,
						alpha=0.5)

	ax1.scatter( x_best_dv,
				y_best_dv,
				s=6,
				marker="*",
				lw=5,
				color='r',
				linewidth=5)

	ax1.scatter(x_temp_best,y_temp_best,s=6,marker="*",lw=5,color='b',linewidth=5)

	cbar = plt.colorbar(data, cmap=cmap)
	formatter = matplotlib.ticker.ScalarFormatter(useOffset=False)
	ax1.xaxis.set_major_formatter(formatter)
	ax1.yaxis.set_major_formatter(formatter)
	ax1.get_yaxis().set_tick_params(direction='out')
	ax1.get_xaxis().set_tick_params(direction='out')
	ax1.set_xlabel('Time since initial epoch [day] \n Initial departure epoch = '			  \
	 + str(first_departure_epoch) + ' [mjd]', fontsize=10)
	ax1.set_ylabel('T$_{ToF}$ [day]', fontsize=10)
	cbar.ax.set_ylabel('Total transfer $\Delta V$ [km/s]', rotation=270, fontsize=10, labelpad=20)
	# plt.title(str(timestep), fontsize=10)


	plt.xlim(0, max_departure_epoch)
	plt.ylim(0, max_time_of_flight)

	if config['title'] == "True":
	 	plt.title("Porkchop plot of TLE elements " + str(a) + " to " + str(b) +					  \
	 			  ". Timestep: " + str(timestep) + " seconds",												  \
	 			  fontsize=10, y=1.02)


	# ax1.annotate(best_dv, xy=(x_best_dv,y_best_dv), xytext=(x_best_dv*.9,y_best_dv*1.1), arrowprops=dict(facecolor='red', shrink=0.05) )


	# ax1.annotate(temp_best, xy=(x_temp_best, y_temp_best), xytext=(x_temp_best*1.1,y_temp_best*0.9),
 #            arrowprops=dict(facecolor='blue', shrink=0.04),
 #            )

	ax1.annotate(best_dv, xy=(x_best_dv,y_best_dv), xytext=(x_best_dv+max_departure_epoch*0.05,y_best_dv+max_time_of_flight*0.05), arrowprops=dict(facecolor='black', shrink=0.05), )
	ax1.annotate(temp_best, xy=(x_temp_best,y_temp_best), xytext=(x_temp_best+ max_departure_epoch*.05,y_temp_best- max_time_of_flight*0.05), arrowprops=dict(facecolor='blue', shrink=0.04))

	plt.tight_layout()
	plt.savefig(config["output_directory"] + "/" + config["scan_figure"] + "_" + str(x) + ".png", \
	            dpi=config["figure_dpi"])

	plt.xlim((x_best_dv-0.0005787037037),(x_best_dv+0.0005787037037))
	plt.ylim((y_best_dv-0.0005787037037),(y_best_dv+0.0005787037037))

	ax1.annotate(best_dv, xy=(x_best_dv,y_best_dv), xytext=(x_best_dv+(x_best_dv-50/86400)*0.0001,y_best_dv+(y_best_dv+50/86400)*0.0001), arrowprops=dict(facecolor='black', shrink=0.05), )
	ax1.annotate(temp_best, xy=(x_temp_best,y_temp_best), xytext=(x_temp_best+ (x_best_dv-50/86400)*0.0001,y_temp_best- (y_best_dv+50/86400)*0.0001), arrowprops=dict(facecolor='blue', shrink=0.04))

	# plt.tight_layout()
	plt.savefig(config["output_directory"] + "/" + config["scan_figure"] + "_a_" + str(x) + ".png", \
	            dpi=config["figure_dpi"])


print ""
print "Figure generated successfully!"

# Stop timer
end_time = time.time( )
print ""
print "------------------------------------------------------------------"
print ""
# Print elapsed time

print ""
print "------------------------------------------------------------------"
print "			Exited successfully!                     "
print "                              "
print "			Script time: " + str("{:,g}".format(end_time - start_time)) + "s"
print "------------------------------------------------------------------"
print ""
