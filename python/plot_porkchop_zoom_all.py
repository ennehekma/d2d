'''
Copyright (c) 2014-2016, Kartik Kumar (me@kartikkumar.com)
Copyright (c) 2016, Enne Hekma (ennehekma@gmail.com)
All rights reserved.
'''

# Set up modules and packages
# Plotting
import matplotlib
matplotlib.use('Agg')
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
	                    FROM lambert_scanner_zoom_results 											  \
	 					ORDER BY transfer_delta_v ASC 											  \
	 					LIMIT 10 ",																  \
	                    database )

	a = (best['departure_object_id'][0])
	b = (best['arrival_object_id'][0])
elif config['objects']==["all"]:
	allcombinations = pd.read_sql("SELECT departure_object_id, arrival_object_id, count(*) \
								   AS number_of_occurences \
								   FROM lambert_scanner_zoom_results \
								   GROUP BY departure_object_id, arrival_object_id \
								   ORDER BY number_of_occurences",
								   database)	
	# print allcombinations

else:
	a = config['objects'][0]
	b = config['objects'][1]

# print len(allcombinations)
for x in xrange(0, len(allcombinations) ):
# for x in xrange(8, 9 ):
	print allcombinations['departure_object_id'][x]
	a = allcombinations['departure_object_id'][x]
	b = allcombinations['arrival_object_id'][x]
	number_of_occurences = allcombinations['number_of_occurences'][x]



	print "Porkchop plot figure being generated for transfer between TLE objects", a, "and", b, "..."

	raw_data = pd.read_sql_query("	SELECT 	time_of_flight,									  		  \
											departure_epoch,			  				  		 	  \
											transfer_delta_v, 								  		  \
											zoom_loop_counter		  						  		  \
											FROM 	lambert_scanner_zoom_results \
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
	first_departure_epoch = 2457400
	raw_data['departure_epoch'] = raw_data['departure_epoch'] - first_departure_epoch
	raw_data['time_of_flight'] = raw_data['time_of_flight']/86400
	number_of_iterations = int(raw_data['iteration'][raw_data['iteration'].argmax()])
	# print number_of_iterations
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
	for x in [6]:
	# for x in xrange(0,number_of_iterations+1):

		print "Plotting scatterplot ",x+1
		timestep = timestep/2
		print "Timestep :",timestep

		bestuptillnow = raw_data.loc[raw_data['iteration']<=x]
		
		tempstring=raw_data.loc[raw_data['iteration']==x]
		tempstringold=raw_data.loc[raw_data['iteration']==x-1]
		tempstringoldold=raw_data.loc[raw_data['iteration']==x-2]




		best_dv = str(round((raw_data['transfer_delta_v'][raw_data['transfer_delta_v'].argmin()])*1000,1))
		x_best_dv = raw_data['departure_epoch'][raw_data['transfer_delta_v'].argmin()]
		y_best_dv = raw_data['time_of_flight'][raw_data['transfer_delta_v'].argmin()]+x_best_dv

		temp_best = str(round((bestuptillnow['transfer_delta_v'][bestuptillnow['transfer_delta_v'].argmin()])*1000,1))
		x_temp_best = bestuptillnow['departure_epoch'][bestuptillnow['transfer_delta_v'].argmin()]
		y_temp_best = bestuptillnow['time_of_flight'][bestuptillnow['transfer_delta_v'].argmin()]

		bestuptillnow['arrival_epoch'] = bestuptillnow['departure_epoch']+bestuptillnow['time_of_flight']
		
		# print bestuptillnow
		bestuptillnow.sort_values(['arrival_epoch'] )
		# print bestupti	llnow
		bestuptillnow.reindex()


		# Plot porkchop plot
		cmap = plt.get_cmap(config['colormap'])
		fig=plt.figure()
		ax1 = fig.add_subplot(111)
		earliest_x = bestuptillnow['departure_epoch'][bestuptillnow['arrival_epoch'].argmin()]
		earliest_y = bestuptillnow['arrival_epoch'][bestuptillnow['arrival_epoch'].argmin()]
		earliest_dv = bestuptillnow['transfer_delta_v'][bestuptillnow['arrival_epoch'].argmin()]
		ax1.annotate(str(earliest_dv)+" "+str(earliest_y),xy=(1,14))
		ax1.scatter(earliest_x,earliest_y,s=40,lw=.4,edgecolor='r',facecolor='none')

		temp = bestuptillnow.loc[bestuptillnow['arrival_epoch']<=y_best_dv]
		for x in xrange(0,100):
			temp2 = temp.loc[temp['arrival_epoch']<=y_best_dv-x]
			temp3 = temp2.loc[temp2['arrival_epoch']>=y_best_dv-1-x]
			if temp3.empty==False:
				# print temp3
				current_x = temp3['departure_epoch'][temp3['transfer_delta_v'].argmin()]
				current_y = temp3['arrival_epoch'][temp3['transfer_delta_v'].argmin()]
				ax1.scatter(current_x,current_y,s=40,edgecolor='k',facecolor='none',lw=.4)
				current_dv = temp3['transfer_delta_v'][temp3['transfer_delta_v'].argmin()]
				ax1.annotate(str(round(current_dv*1000,1))+" "+str(current_y),xy=(current_x+1,current_y))
				# ax1.scatter(temp3['departure_epoch'],temp3['arrival_epoch'],s=40,edgecolor='k',facecolor='none',lw=.4)
				
			

		data = ax1.scatter(	tempstring['departure_epoch'],
							tempstring['time_of_flight']+tempstring['departure_epoch'],
							c=tempstring['transfer_delta_v'],
							cmap=cmap,
							s=2,
							lw=0)
		ax1.scatter(		tempstringold['departure_epoch'],
							tempstringold['time_of_flight']+tempstringold['departure_epoch'],
							c=tempstringold['transfer_delta_v'],
							cmap=cmap,
							s=0.5,
							lw=0)
		ax1.scatter( 		tempstringoldold['departure_epoch'],
							tempstringoldold['time_of_flight']+tempstringoldold['departure_epoch'],
							c=tempstringoldold['transfer_delta_v'],
							cmap=cmap,
							s=0.1,
							lw=0,
							alpha=0.5)
		
		ax1.scatter(x_best_dv,y_best_dv,s=6,marker="*",lw=5,color='b',linewidth=5)

		ax1.scatter(x_best_dv,y_best_dv,s=6,marker="*",lw=5,color='b',linewidth=5)

		ax1.plot([0,14],[0,14], color='k', linewidth=0.1, linestyle='--')
		ax1.plot([0,14],[2,16], color='k', linewidth=0.1, linestyle='--')


		# cbar = plt.colorbar(data, cmap=cmap)
		formatter = matplotlib.ticker.ScalarFormatter(useOffset=False)
		ax1.xaxis.set_major_formatter(formatter)
		ax1.yaxis.set_major_formatter(formatter)
		ax1.get_yaxis().set_tick_params(direction='out')
		ax1.get_xaxis().set_tick_params(direction='out')
		ax1.set_xlabel('Departure epoch [days after initial epoch] \n Initial epoch = '			  \
		 + str(first_departure_epoch) + ' [mjd]', fontsize=10)
		ax1.set_ylabel('Arrival epoch [days after initial epoch]', fontsize=10)
		# cbar.ax.set_ylabel('Total transfer $\Delta V$ [km/s]', rotation=270, fontsize=10, labelpad=20)
		# plt.title(str(timestep), fontsize=10)


		# # plt.xlim(0, max_departure_epoch)
		# # plt.ylim(0, max_time_of_flight)
		plt.xlim(0, 14)
		plt.ylim(0, 16)

		if config['title'] == "True":
		 	plt.title("Porkchop plot of TLE elements " + str(a) + " to " + str(b) +					  \
		 			  ". Timestep: " + str(timestep) + " seconds. Number of points: " + str(number_of_occurences),												  \
		 			  fontsize=10, y=1.02)


		# ax1.annotate(best_dv, xy=(x_best_dv,y_best_dv), xytext=(x_best_dv*.9,y_best_dv*1.1), arrowprops=dict(facecolor='red', shrink=0.05) )


		# ax1.annotate(temp_best, xy=(x_temp_best, y_temp_best), xytext=(x_temp_best*1.1,y_temp_best*0.9),
	 #            arrowprops=dict(facecolor='blue', shrink=0.04),
	 #            )

		ax1.annotate(best_dv, xy=(x_best_dv,y_best_dv), xytext=(x_best_dv+max_departure_epoch*0.05,y_best_dv+max_time_of_flight*0.05), arrowprops=dict(facecolor='black', shrink=0.05), )
		# ax1.annotate(temp_best, xy=(x_temp_best,y_temp_best), xytext=(x_temp_best+ max_departure_epoch*.05,y_temp_best- max_time_of_flight*0.05), arrowprops=dict(facecolor='blue', shrink=0.04))

		plt.tight_layout()
		plt.savefig(config["output_directory"] + "/" + config["scan_figure"] + "_" + str(a) + "_" + str(b) + "_" + str(number_of_occurences) + "stuks.png", \
		            dpi=config["figure_dpi"])

		print a, " to ",b, " done!"
		plt.close()
		# plt.xlim((x_best_dv-0.0005787037037),(x_best_dv+0.0005787037037))
		# plt.ylim((y_best_dv-0.0005787037037),(y_best_dv+0.0005787037037))

		# ax1.annotate(best_dv, xy=(x_best_dv,y_best_dv), xytext=(x_best_dv+(x_best_dv-50/86400)*0.0001,y_best_dv+(y_best_dv+50/86400)*0.0001), arrowprops=dict(facecolor='black', shrink=0.05), )
		# ax1.annotate(temp_best, xy=(x_temp_best,y_temp_best), xytext=(x_temp_best+ (x_best_dv-50/86400)*0.0001,y_temp_best- (y_best_dv+50/86400)*0.0001), arrowprops=dict(facecolor='blue', shrink=0.04))

		# # plt.tight_layout()
		# plt.savefig(config["output_directory"] + "/" + config["scan_figure"] + "_a_" + str(x) + ".png", \
		#             dpi=config["figure_dpi"])


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
