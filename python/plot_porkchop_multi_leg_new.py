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



data_multi = pd.read_sql_query("	SELECT transfer_id_1,mission_duration,(launch_epoch-2457542.5)*24*3600,total_transfer_delta_v  \
									FROM lambert_scanner_multi_leg_transfers 					  \
									WHERE sequence_id = 1",							              \
										database)
# data_departure = pd.read_sql_query("SELECT transfer_id,(departure_epoch-2457542.5)*24*3600 \
# 									FROM lambert_scanner_transfers 					  \
# 									WHERE transfer_id < 2602 \
# 									and leg_id=1",							              \
# 										database)
data_multi.columns=['transfer_id','mission_duration','departure_epoch','total_transfer_delta_v']
# sys.exit()
# data_departure.columns=['transfer_id_1','departure_epoch']
# data_departure['departure_epoch'] = np.round(data_departure['departure_epoch'])
data_multi['mission_duration'] = np.round(data_multi['mission_duration'])
data_multi['departure_epoch'] = np.round(data_multi['departure_epoch'])
# new = pd.merge(data_departure,data_multi, how='left', on=['transfer_id_1'])
data_multi = data_multi.sort_values(by=['departure_epoch','mission_duration','total_transfer_delta_v']).drop_duplicates(subset=['departure_epoch','mission_duration'], keep='first').reset_index()

departure_epochs = data_multi['departure_epoch'].drop_duplicates()
times_of_flight = data_multi['mission_duration'].drop_duplicates()
transfer_delta_vs = data_multi['total_transfer_delta_v']
print (departure_epochs),"\n",(times_of_flight),"\n", (transfer_delta_vs),"\n"
print "Departure epochs: ",len(departure_epochs),"\n"
print "Mission duration points: ",len(times_of_flight),"\n"
print "DeltaV points: " ,len(transfer_delta_vs),"\n"

z = np.array(transfer_delta_vs).reshape(len(departure_epochs), len(times_of_flight))
x1, y1 = np.meshgrid(times_of_flight,departure_epochs)

cutoff = 8
# Cutoff:
z = np.ma.array(z, mask=z > cutoff)
print "Cutoff is applied of ", cutoff, "km/s"
print "Inputs are correct, plotting will now commence"

# Plot porkchop plot
cmap = plt.get_cmap(config['colormap'])
fig=plt.figure()
ax1 = fig.add_subplot(111)

data = ax1.contourf(y1,x1,z,cmap=cmap)
# data = ax1.pcolormesh(y1,x1,z,cmap=cmap)
cbar = plt.colorbar(data, cmap=cmap)
formatter = matplotlib.ticker.ScalarFormatter(useOffset=False)
ax1.xaxis.set_major_formatter(formatter)
ax1.yaxis.set_major_formatter(formatter)
# ax1.set_xlim([0,10000])
# ax1.set_ylim([1000,31000])
# ax1.set_xlim([0,10000])
# ax1.set_ylim([1000,31000])

ax1.get_yaxis().set_tick_params(direction='out')
ax1.get_xaxis().set_tick_params(direction='out')
ax1.set_xlabel('Time since initial epoch [seconds]' 					  \
				, fontsize=13)
			   # + str(first_departure_epoch) + ' [mjd]', fontsize=13)
ax1.set_ylabel('Total mission duration [seconds]', fontsize=13)
cbar.ax.set_ylabel('Total transfer $\Delta V$ [km/s]', rotation=270, fontsize=13, labelpad=20)
plt.title("Lambert "+str(cutoff)+" km/s")
plt.tight_layout()
plt.savefig(config["output_directory"] + "/" +  "met_dep.png", 					  \
            dpi=config["figure_dpi"])

plt.clf()
sys.exit()
departure_epochs = leg2['departure_2'].drop_duplicates()
times_of_flight = leg2['tof_2'].drop_duplicates()
transfer_delta_vs = leg2['dv_2']
z = np.array(transfer_delta_vs).reshape(len(departure_epochs), len(times_of_flight))
x1, y1 = np.meshgrid(times_of_flight,departure_epochs)

cutoff = 6
for i in xrange(0,len(departure_epochs)):
	for j in xrange(0,len(times_of_flight)):
		if math.isnan(z[i][j]) == True:
			z[i][j] = cutoff+1
			# print z[i][j]
		elif z[i][j] > cutoff:
			z[i][j] =  cutoff


# datapoints = np.size(z)
# failures = np.count_nonzero(np.isnan(z))
print "Inputs are correct, plotting will now commence"
# Plot porkchop plot
cmap = plt.get_cmap(config['colormap'])
fig=plt.figure()
ax1 = fig.add_subplot(111)
# data = ax1.contourf(y1,x1,z,cmap=cmap)
data = ax1.pcolormesh(y1,x1,z,cmap=cmap)
cbar = plt.colorbar(data, cmap=cmap)
formatter = matplotlib.ticker.ScalarFormatter(useOffset=False)
ax1.xaxis.set_major_formatter(formatter)
ax1.yaxis.set_major_formatter(formatter)
ax1.get_yaxis().set_tick_params(direction='out')
ax1.get_xaxis().set_tick_params(direction='out')
ax1.set_xlabel('Time since initial epoch [seconds]' 					  \
				, fontsize=13)
			   # + str(first_departure_epoch) + ' [mjd]', fontsize=13)
ax1.set_ylabel('T$_{ToF}$ (cummulative) [seconds]', fontsize=13)
cbar.ax.set_ylabel('Total transfer $\Delta V$ [km/s]', rotation=270, fontsize=13, labelpad=20)

plt.tight_layout()
plt.savefig(config["output_directory"] + "/" +  "pork_chop_plot_leg2.png", 					  \
            dpi=config["figure_dpi"])

plt.clf()

################################################################################################33333
# sys.exit()
# # Create porkchopplot after second leg with deltaV to get to that part
# # Extract correlationi arrival-dv_1
# leg1['arrival_1'] = leg1['departure_1']+leg1['tof_1']
# arr_dv = leg1.sort_values(by=['arrival_1','dv_1']).drop_duplicates('arrival_1', keep='first')
# arr_dv['departure_2'] = arr_dv['arrival_1']
# new2 = pd.merge(leg2, arr_dv, how='left', on=['departure_2'])
# new2['dv_tot'] = new2['dv_1'] + new2['dv_2']
# departure_epochs = leg1['dep'].drop_duplicates()
# times_of_flight = leg1['tof'].drop_duplicates()
# transfer_delta_vs = new2['dv_tot']
# print new2

# Create total mission porkchop plot, departure epochs of leg 1 and cummulative time-of-flights.
leg1['departure_2'] = leg1['departure_1']+leg1['tof_1']
print leg1,leg2
print leg1['departure_2']
print leg2['departure_2'].drop_duplicates()
# sys.exit()


total_mission = pd.merge(leg1,leg2, how='left', on=['departure_2'])
total_mission['tof_total'] = total_mission['tof_1'] + total_mission['tof_2']
print total_mission
total_mission['dv_total'] = total_mission['dv_1'] + total_mission['dv_2']

total_mission_good = total_mission.sort_values(by=['departure_1','tof_total','dv_total']).drop_duplicates(subset=['tof_total','departure_1'], keep='first')
# print total_mission_good
count_nan = len(total_mission_good) - total_mission_good.count()
# print count_nan
# sys.exit(1)
departure_epochs = leg1['departure_1'].drop_duplicates()
times_of_flight = total_mission_good['tof_total'].drop_duplicates()
transfer_delta_vs = total_mission_good['dv_total']
print "These are the inputs given:"
# Really beginning of plotting script, 3 lists should be define beforehand and are printed now.
print "\nDeparture epochs: \n", departure_epochs
print "\nTimes of flight: \n", times_of_flight
print "\nTransfer delta vs: \n", transfer_delta_vs
# sys.exit()
z = np.array(transfer_delta_vs).reshape(len(departure_epochs), len(times_of_flight))
x1, y1 = np.meshgrid(times_of_flight,departure_epochs)
# datapoints = np.size(z)
# failures = np.count_nonzero(np.isnan(z))
print "Inputs are correct, plotting will now commence"

# # Add cutoff dV if user input requires to do so
# if config['transfer_deltaV_cutoff'] != 0:
	# cutoff = config['transfer_deltaV_cutoff']
cutoff = 6
for i in xrange(0,len(departure_epochs)):
	for j in xrange(0,len(times_of_flight)):
		if math.isnan(z[i][j]) == True:
			z[i][j] = cutoff+1
			# print z[i][j]
		elif z[i][j] > cutoff:
			z[i][j] =  cutoff

# Plot porkchop plot
cmap = plt.get_cmap(config['colormap'])
fig=plt.figure()
ax1 = fig.add_subplot(111)
# data = ax1.contourf(y1,x1,z,cmap=cmap)
data = ax1.pcolormesh(y1,x1,z,cmap=cmap)
cbar = plt.colorbar(data, cmap=cmap)
formatter = matplotlib.ticker.ScalarFormatter(useOffset=False)
ax1.xaxis.set_major_formatter(formatter)
ax1.yaxis.set_major_formatter(formatter)
ax1.get_yaxis().set_tick_params(direction='out')
ax1.get_xaxis().set_tick_params(direction='out')
ax1.set_xlabel('Time since initial epoch [seconds]' 					  \
				, fontsize=13)
			   # + str(first_departure_epoch) + ' [mjd]', fontsize=13)
ax1.set_ylabel('T$_{ToF}$ (cummulative) [seconds]', fontsize=13)
cbar.ax.set_ylabel('Total transfer $\Delta V$ [km/s]', rotation=270, fontsize=13, labelpad=20)
# plt.ticklabel_format(style='sci', axis='x', scilimits=(0,0))

# if config['title'] == "True":
# 	if config['transfer_deltaV_cutoff'] == 0:
# 	 	plt.title("Porkchop plot of TLE elements " + str(a) + " to " + str(b) + 				  \
# 	 			  ".\n Number of datapoints: " + str(datapoints) + " (" + 						  \
# 	 			  str( len( departure_epochs ) ) + "x" + str(len(times_of_flight)) + 			  \
# 	 			  "). Total failures: " + str(failugodres),		 								  \
# 	 			  fontsize=10, y=1.02)
# 	else:
# 		plt.title("Porkchop plot of TLE elements " + str(a) + " to " + str(b) +				 	  \
# 				  "\n with a cutoff $\Delta V$ of " + str(config['transfer_deltaV_cutoff']) + 	  \
# 				  "\n Number of datapoints: " + str(datapoints) + " (" + 						  \
# 				  str(len(departure_epochs)) + "x" + str(len(times_of_flight)) + ") " +			  \
# 				  "Total failures: " + str(failures), 		  									  \
# 				  fontsize=10, y=1.02)

plt.tight_layout()
plt.savefig(config["output_directory"] + "/" + config["scan_figure"] + "total.png", 					  \
            dpi=config["figure_dpi"])
# plt.savefig(filelocation1+ "goed.png", 					  \
            # dpi=config["figure_dpi"])

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



# print times_of_flight).drop_duplicates(), np.round((departure_epochs-)*24*3600), transfer_delta_vs


# times_of_flight = times_of_flight / 86400

# z = np.array(transfer_delta_vs).reshape(len(departure_epochs), len(times_of_flight))
# x1, y1 = np.meshgrid(times_of_flight,departure_epochs)
# datapoints = np.size(z)
# failures = np.count_nonzero(np.isnan(z))


# print "Fetching porkchop plot data from temporary csv file ..."

# filelocation1 = "/home/enne/Dropbox/_Master Space Exploration/Dinamica Internship/multi_leg_test_files/leg1_dep5_arr11.csv"
# filelocation2 = "/home/enne/Dropbox/_Master Space Exploration/Dinamica Internship/multi_leg_test_files/leg2_dep11_arr12.csv"

# leg1 = pd.read_csv(filelocation1,header=None)
# leg1.columns=['departure_1','tof_1','dv_1']
# leg2 = pd.read_csv(filelocation2,header=None)
# leg2.columns=['departure_2','tof_2','dv_2']

# sys.exit()
# # print leg1, leg2
