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
leg_id = 1
times_of_flight = np.round(pd.read_sql_query(" SELECT DISTINCT time_of_flight            \
                        FROM lambert_scanner_results            \
                        WHERE leg_id =" + str(leg_id),            \
                    database)).drop_duplicates()
departure_epochs = pd.read_sql_query("  SELECT DISTINCT departure_epoch               \
                    FROM lambert_scanner_results                \
                    WHERE leg_id =" + str(leg_id),                \
                    database)-2400000.5 # change to Modified Julian Date
first_departure_epoch = departure_epochs['departure_epoch'][0]
departure_epochs['departure_epoch'] = np.round((departure_epochs['departure_epoch'] - first_departure_epoch)*24*3600)

atom_data = pd.read_sql_query("SELECT DISTINCT lambert_scanner_results.departure_object_id, "
							  "				   lambert_scanner_results.arrival_object_id, "
							  "				   lambert_scanner_results.time_of_flight, "
							  "				   lambert_scanner_results.departure_epoch, "
							  "				   lambert_scanner_results.leg_id, "
                              "                lambert_scanner_results.transfer_delta_v, "
							  "				   (atom_scanner_results.atom_transfer_delta_v)"
                              "FROM            atom_scanner_results "
                              "INNER JOIN      lambert_scanner_results "
                              "ON              lambert_scanner_results.transfer_id "
                              "                = atom_scanner_results.lambert_transfer_id "
                              "WHERE           atom_scanner_results.atom_transfer_delta_v<4 "
                              "AND             lambert_scanner_results.departure_object_id = 16615 "
                              "AND             lambert_scanner_results.leg_id = 1 "
                              "AND             lambert_scanner_results.arrival_object_id = 21610;",
							  database)
first_departure_epoch = atom_data['departure_epoch'][0]
atom_data['departure_epoch'] = np.round((atom_data['departure_epoch'] - first_departure_epoch)*24*3600)
atom_data['time_of_flight'] = np.round(atom_data['time_of_flight'])


# leg1 = pd.read_sql_query("  SELECT time_of_flight,departure_epoch,transfer_delta_v                \
#                                         FROM lambert_scanner_results                              \
#                                         WHERE departure_object_id =" + str(16615) + "                 \
#                                         AND arrival_object_id =" + str(21610) +"                      \
#                                         AND leg_id =" + str(1),                           \
#                                         database)


print times_of_flight
print atom_data
# sys.exit()
transfer_delta_vs = np.ones(len(times_of_flight)*len(departure_epochs))*4.0100001
z = np.array(transfer_delta_vs).reshape(len(departure_epochs), len(times_of_flight))
z_lambert = np.array(transfer_delta_vs).reshape(len(departure_epochs), len(times_of_flight))


counter = 0
for i in xrange(0,len(departure_epochs)):
    for j in xrange(0,len(times_of_flight)):
        total = (i*len(times_of_flight))+j
        dV = config['transfer_deltaV_cutoff']
        # print atom_data['time_of_flight'][counter]
        if counter==len(atom_data):
            continue
        # print times_of_flight['time_of_flight'][j]
        # print atom_data['departure_epoch'][counter],departure_epochs['departure_epoch'][i]
        if (atom_data['time_of_flight'][counter]==times_of_flight['time_of_flight'][j] and atom_data['departure_epoch'][counter]==departure_epochs['departure_epoch'][i]
            and atom_data['atom_transfer_delta_v'][counter]<4):
            # print i,j, 'hello', counter
            dV = atom_data['atom_transfer_delta_v'][counter]
            dV_lambert = atom_data['transfer_delta_v'][counter]
            counter = counter + 1
            z.put(total,dV)
            z_lambert.put(total,dV_lambert)
# print z
# sys.exit()

# z.put(10,2)
# print type(z)
x1, y1 = np.meshgrid(times_of_flight,departure_epochs)
z = np.ma.array(z, mask=z > 4.01)

# print x1,y1,z

cmap = plt.get_cmap(config['colormap'])
fig=plt.figure()
ax1 = fig.add_subplot(111)
data = ax1.pcolormesh(y1,x1,z,cmap=cmap, vmin=2.0, vmax=4.0)
# print type(data)

# data = ax1.contourf(y1,x1,z,cmap=cmap)
bounds=[2.0,2.25,2.5,2.75,3,3.25,3.5,3.75,4.0]
# norm = matplotlib.colors.BoundaryNorm(bounds, cmap.N)
# norm = matplotlib.colors.Normalize(vmin=2, vmax=4.1)
cbar = plt.colorbar(data, cmap=cmap,ticks=bounds)
formatter = matplotlib.ticker.ScalarFormatter(useOffset=False)
ax1.xaxis.set_major_formatter(formatter)
ax1.yaxis.set_major_formatter(formatter)
ax1.set_xlim([0,10000])
ax1.set_ylim([14000,51000])
ax1.get_yaxis().set_tick_params(direction='out')
ax1.get_xaxis().set_tick_params(direction='out')
ax1.set_xlabel('Time since initial epoch [seconds]'                     \
              , fontsize=13)
             # + str(first_departure_epoch) + ' [mjd]', fontsize=13)
ax1.set_ylabel('T$_{ToF}$ (cummulative) [seconds]', fontsize=13)
cbar.ax.set_ylabel('Total transfer $\Delta V$ [km/s]', rotation=270, fontsize=13, labelpad=20)
plt.title("Atom")
plt.tight_layout()
# plt.savefig(config["output_directory"] + "/" +  "pork_chop_plot_leg1.png",                      \
#             dpi=config["figure_dpi"])



plt.savefig( config['output_directory']+ "fraleg1_atom_klein.png",             \
            dpi=config["figure_dpi"])
# sys.exit()
plt.clf()

z_lambert = np.ma.array(z_lambert, mask=z_lambert > 4.01)


cmap = plt.get_cmap(config['colormap'])
fig=plt.figure()
ax1 = fig.add_subplot(111)
data = ax1.pcolormesh(y1,x1,z_lambert,cmap=cmap, vmin=2.0, vmax=4.0)
# print "z",z,"z_lambert",z_lambert
# data = ax1.contourf(y1,x1,z,cmap=cmap)
cbar = plt.colorbar(data, cmap=cmap, ticks=bounds)
formatter = matplotlib.ticker.ScalarFormatter(useOffset=False)
ax1.xaxis.set_major_formatter(formatter)
ax1.yaxis.set_major_formatter(formatter)
ax1.set_xlim([0,10000])
ax1.set_ylim([14000,51000])
ax1.get_yaxis().set_tick_params(direction='out')
ax1.get_xaxis().set_tick_params(direction='out')
ax1.set_xlabel('Time since initial epoch [seconds]'                     \
              , fontsize=13)
             # + str(first_departure_epoch) + ' [mjd]', fontsize=13)
ax1.set_ylabel('T$_{ToF}$ (cummulative) [seconds]', fontsize=13)
cbar.ax.set_ylabel('Total transfer $\Delta V$ [km/s]', rotation=270, fontsize=13, labelpad=20)
plt.title("Lambert")
plt.tight_layout()
# plt.savefig(config["output_directory"] + "/" +  "pork_chop_plot_leg1.png",                      \
#             dpi=config["figure_dpi"])

# plt.clf()


plt.savefig( config['output_directory']+ "fraleg1_lambert_klein.png",             \
            dpi=config["figure_dpi"])

plt.clf()

z_lambert = np.ma.array(z_lambert, mask=z_lambert > 4.01)
diff = z-z_lambert
# print diff[diff<0.0])
# print "enne",type(diff)
# print diff.flatten()
plt.hist(diff.compressed())
plt.savefig( config['output_directory']+ "fraleg1_hist_klein.png",             \
            dpi=config["figure_dpi"])


# sys.exit()
cmap = plt.get_cmap(config['colormap'])
fig=plt.figure()
ax1 = fig.add_subplot(111)
data = ax1.pcolormesh(y1,x1,diff,cmap=cmap)
# print "z",z,"z_lambert",z_lambert
# data = ax1.contourf(y1,x1,z,cmap=cmap)
cbar = plt.colorbar(data, cmap=cmap)
formatter = matplotlib.ticker.ScalarFormatter(useOffset=False)
ax1.xaxis.set_major_formatter(formatter)
ax1.yaxis.set_major_formatter(formatter)
ax1.set_xlim([0,10000])
ax1.set_ylim([14000,51000])
ax1.get_yaxis().set_tick_params(direction='out')
ax1.get_xaxis().set_tick_params(direction='out')
ax1.set_xlabel('Time since initial epoch [seconds]'                     \
              , fontsize=13)
             # + str(first_departure_epoch) + ' [mjd]', fontsize=13)
ax1.set_ylabel('T$_{ToF}$ (cummulative) [seconds]', fontsize=13)
cbar.ax.set_ylabel('Total transfer $\Delta V$ [km/s]', rotation=270, fontsize=13, labelpad=20)
plt.title("Atom - Lambert")
plt.tight_layout()
# plt.savefig(config["output_directory"] + "/" +  "pork_chop_plot_leg1.png",                      \
#             dpi=config["figure_dpi"])

# plt.clf()


plt.savefig( config['output_directory']+ "fraleg1_atom-lambert_klein.png",             \
            dpi=config["figure_dpi"])

print ""
print "Figure generated successfully!"
# d = np.random.rand(10, 10)
# d[2, 2], d[3, 5] = np.nan, np.nan
# # and in your case you actually have masked values too:
# d = np.ma.array(d, mask=d < .2)

# print d

# d[np.isnan(d)] = 1
# plt.gca().patch.set_color('.99999')
# plt.pcolormesh(d)
# # plt.contourf(d)

# print "Porkchop plot figure being generated for transfer between TLE objects:\n",
# for i in xrange(0,len(config['sequence'])-1):
# 	print config['sequence'][i]
# print config['sequence'][len(config['sequence'])-1]

# numer_of_legs_in_database = len(pd.read_sql_query("	SELECT DISTINCT(leg_id)\
# 										FROM lambert_scanner_results",
# 										database))

# # if numer_of_legs_in_database!=len(config['sequence'])-1:
# # 	print "Warning: sequence length given is not the same as the amount of legs in database!"
# # 	print "Exiting program, edit input file"
# # 	sys.exit()


# # print "Number of legs:"
# # print numer_of_legs_in_database

# # # For loop over all legs in database
# # for leg_id in xrange(1,len(config['sequence'])):
# # 	print leg_id
# leg_id = 1
# a= config['sequence'][0]
# b= config['sequence'][1]


# # times_of_flight = np.round(pd.read_sql_query("	SELECT DISTINCT time_of_flight 					  \
# # 												FROM lambert_scanner_results					  \
# # 												WHERE leg_id =" + str(leg_id),					  \
# # 										database)).drop_duplicates()
# # departure_epochs = pd.read_sql_query("	SELECT DISTINCT departure_epoch 						  \
# # 										FROM lambert_scanner_results							  \
# # 										WHERE leg_id =" + str(leg_id),							  \
# # 										database)-2400000.5 # change to Modified Julian Date
# # transfer_delta_vs = pd.read_sql_query("	SELECT transfer_delta_v 								  \
# # 										FROM lambert_scanner_results 							  \
# # 										WHERE departure_object_id =" + str(a) + "				  \
# # 										AND arrival_object_id =" + str(b) +"					  \
# # 										AND leg_id =" + str(leg_id),							  \
# # 										database)
# # first_departure_epoch = departure_epochs['departure_epoch'][0]
# # departure_epochs = (departure_epochs - first_departure_epoch)*24*3600


# leg1 = pd.read_sql_query("	SELECT time_of_flight,departure_epoch,transfer_delta_v		  		  \
# 										FROM lambert_scanner_results 							  \
# 										WHERE departure_object_id =" + str(16615) + "				  \
# 										AND arrival_object_id =" + str(21610) +"					  \
# 										AND leg_id =" + str(1),							  \
# 										database)
# # print leg1
# leg1['time_of_flight'] = np.round(leg1['time_of_flight'])
# enne = leg1['departure_epoch'][0]
# leg1['departure_epoch'] = np.round((leg1['departure_epoch']-leg1['departure_epoch'][0])*3600*24)
# leg1.columns=['tof_1','departure_1','dv_1']


# leg2 = pd.read_sql_query("	SELECT time_of_flight,departure_epoch,transfer_delta_v		  		  \
# 										FROM lambert_scanner_results 							  \
# 										WHERE departure_object_id =" + str(21610) + "				  \
# 										AND arrival_object_id =" + str(20443) +"					  \
# 										AND leg_id =" + str(2),							  \
# 										database)

# # print leg1,leg2
# # sys.exit()
# leg2['time_of_flight'] = np.round(leg2['time_of_flight'])
# leg2['departure_epoch']= np.round((leg2['departure_epoch']-enne)*3600*24)
# leg2.columns=['tof_2','departure_2','dv_2']

# # leg3 = pd.read_sql_query("	SELECT time_of_flight,departure_epoch,transfer_delta_v		  		  \
# # 										FROM lambert_scanner_results 							  \
# # 										WHERE departure_object_id =" + str(12) + "				  \
# # 										AND arrival_object_id =" + str(16) +"					  \
# # 										AND leg_id =" + str(3),							  \
# # 										database)
# # leg3['time_of_flight'] = np.round(leg3['time_of_flight'])
# # leg3['departure_epoch']= np.round((leg3['departure_epoch']-leg3['departure_epoch'][0])*3600*24+600)
# # leg3.columns=['tof_3','departure_3','dv_3']
# ######################################################################################################
# departure_epochs = leg1['departure_1'].drop_duplicates()
# times_of_flight = leg1['tof_1'].drop_duplicates()
# transfer_delta_vs = leg1['dv_1']
# z = np.array(transfer_delta_vs).reshape(len(departure_epochs), len(times_of_flight))
# x1, y1 = np.meshgrid(times_of_flight,departure_epochs)

# cutoff = 6
# for i in xrange(0,len(departure_epochs)):
# 	for j in xrange(0,len(times_of_flight)):
# 		if math.isnan(z[i][j]) == True:
# 			z[i][j] = cutoff+1
# 			# print z[i][j]
# 		elif z[i][j] > cutoff:
# 			z[i][j] =  cutoff

# # datapoints = np.size(z)
# # failures = np.count_nonzero(np.isnan(z))
# print "Inputs are correct, plotting will now commence"
# # Plot porkchop plot
# cmap = plt.get_cmap(config['colormap'])
# fig=plt.figure()
# ax1 = fig.add_subplot(111)
# data = ax1.contourf(y1,x1,z,cmap=cmap)
# cbar = plt.colorbar(data, cmap=cmap)
# formatter = matplotlib.ticker.ScalarFormatter(useOffset=False)
# ax1.xaxis.set_major_formatter(formatter)
# ax1.yaxis.set_major_formatter(formatter)
# ax1.get_yaxis().set_tick_params(direction='out')
# ax1.get_xaxis().set_tick_params(direction='out')
# ax1.set_xlabel('Time since initial epoch [seconds]' 					  \
# 				, fontsize=13)
# 			   # + str(first_departure_epoch) + ' [mjd]', fontsize=13)
# ax1.set_ylabel('T$_{ToF}$ (cummulative) [seconds]', fontsize=13)
# cbar.ax.set_ylabel('Total transfer $\Delta V$ [km/s]', rotation=270, fontsize=13, labelpad=20)

# plt.tight_layout()
# plt.savefig(config["output_directory"] + "/" +  "pork_chop_plot_leg1.png", 					  \
#             dpi=config["figure_dpi"])

# plt.clf()

# departure_epochs = leg2['departure_2'].drop_duplicates()
# times_of_flight = leg2['tof_2'].drop_duplicates()
# transfer_delta_vs = leg2['dv_2']
# z = np.array(transfer_delta_vs).reshape(len(departure_epochs), len(times_of_flight))
# x1, y1 = np.meshgrid(times_of_flight,departure_epochs)

# cutoff = 6
# for i in xrange(0,len(departure_epochs)):
# 	for j in xrange(0,len(times_of_flight)):
# 		if math.isnan(z[i][j]) == True:
# 			z[i][j] = cutoff+1
# 			# print z[i][j]
# 		elif z[i][j] > cutoff:
# 			z[i][j] =  cutoff


# # datapoints = np.size(z)
# # failures = np.count_nonzero(np.isnan(z))
# print "Inputs are correct, plotting will now commence"
# # Plot porkchop plot
# cmap = plt.get_cmap(config['colormap'])
# fig=plt.figure()
# ax1 = fig.add_subplot(111)
# data = ax1.contourf(y1,x1,z,cmap=cmap)
# cbar = plt.colorbar(data, cmap=cmap)
# formatter = matplotlib.ticker.ScalarFormatter(useOffset=False)
# ax1.xaxis.set_major_formatter(formatter)
# ax1.yaxis.set_major_formatter(formatter)
# ax1.get_yaxis().set_tick_params(direction='out')
# ax1.get_xaxis().set_tick_params(direction='out')
# ax1.set_xlabel('Time since initial epoch [seconds]' 					  \
# 				, fontsize=13)
# 			   # + str(first_departure_epoch) + ' [mjd]', fontsize=13)
# ax1.set_ylabel('T$_{ToF}$ (cummulative) [seconds]', fontsize=13)
# cbar.ax.set_ylabel('Total transfer $\Delta V$ [km/s]', rotation=270, fontsize=13, labelpad=20)

# plt.tight_layout()
# plt.savefig(config["output_directory"] + "/" +  "pork_chop_plot_leg2.png", 					  \
#             dpi=config["figure_dpi"])

# plt.clf()

# ################################################################################################33333
# # sys.exit()
# # # Create porkchopplot after second leg with deltaV to get to that part
# # # Extract correlationi arrival-dv_1
# # leg1['arrival_1'] = leg1['departure_1']+leg1['tof_1']
# # arr_dv = leg1.sort_values(by=['arrival_1','dv_1']).drop_duplicates('arrival_1', keep='first')
# # arr_dv['departure_2'] = arr_dv['arrival_1']
# # new2 = pd.merge(leg2, arr_dv, how='left', on=['departure_2'])
# # new2['dv_tot'] = new2['dv_1'] + new2['dv_2']
# # departure_epochs = leg1['dep'].drop_duplicates()
# # times_of_flight = leg1['tof'].drop_duplicates()
# # transfer_delta_vs = new2['dv_tot']
# # print new2

# # Create total mission porkchop plot, departure epochs of leg 1 and cummulative time-of-flights.
# leg1['departure_2'] = leg1['departure_1']+leg1['tof_1']
# print leg1,leg2
# print leg1['departure_2']
# print leg2['departure_2'].drop_duplicates()
# # sys.exit()


# total_mission = pd.merge(leg1,leg2, how='left', on=['departure_2'])
# total_mission['tof_total'] = total_mission['tof_1'] + total_mission['tof_2']
# print total_mission
# total_mission['dv_total'] = total_mission['dv_1'] + total_mission['dv_2']

# total_mission_good = total_mission.sort_values(by=['departure_1','tof_total','dv_total']).drop_duplicates(subset=['tof_total','departure_1'], keep='first')
# # print total_mission_good
# count_nan = len(total_mission_good) - total_mission_good.count()
# # print count_nan
# # sys.exit(1)
# departure_epochs = leg1['departure_1'].drop_duplicates()
# times_of_flight = total_mission_good['tof_total'].drop_duplicates()
# transfer_delta_vs = total_mission_good['dv_total']
# print "These are the inputs given:"
# # Really beginning of plotting script, 3 lists should be define beforehand and are printed now.
# print "\nDeparture epochs: \n", departure_epochs
# print "\nTimes of flight: \n", times_of_flight
# print "\nTransfer delta vs: \n", transfer_delta_vs
# # sys.exit()
# z = np.array(transfer_delta_vs).reshape(len(departure_epochs), len(times_of_flight))
# x1, y1 = np.meshgrid(times_of_flight,departure_epochs)
# # datapoints = np.size(z)
# # failures = np.count_nonzero(np.isnan(z))
# print "Inputs are correct, plotting will now commence"

# # # Add cutoff dV if user input requires to do so
# # if config['transfer_deltaV_cutoff'] != 0:
# 	# cutoff = config['transfer_deltaV_cutoff']
# cutoff = 6
# for i in xrange(0,len(departure_epochs)):
# 	for j in xrange(0,len(times_of_flight)):
# 		if math.isnan(z[i][j]) == True:
# 			z[i][j] = cutoff+1
# 			# print z[i][j]
# 		elif z[i][j] > cutoff:
# 			z[i][j] =  cutoff

# # Plot porkchop plot
# cmap = plt.get_cmap(config['colormap'])
# fig=plt.figure()
# ax1 = fig.add_subplot(111)
# data = ax1.contourf(y1,x1,z,cmap=cmap)
# cbar = plt.colorbar(data, cmap=cmap)
# formatter = matplotlib.ticker.ScalarFormatter(useOffset=False)
# ax1.xaxis.set_major_formatter(formatter)
# ax1.yaxis.set_major_formatter(formatter)
# ax1.get_yaxis().set_tick_params(direction='out')
# ax1.get_xaxis().set_tick_params(direction='out')
# ax1.set_xlabel('Time since initial epoch [seconds]' 					  \
# 				, fontsize=13)
# 			   # + str(first_departure_epoch) + ' [mjd]', fontsize=13)
# ax1.set_ylabel('T$_{ToF}$ (cummulative) [seconds]', fontsize=13)
# cbar.ax.set_ylabel('Total transfer $\Delta V$ [km/s]', rotation=270, fontsize=13, labelpad=20)
# # plt.ticklabel_format(style='sci', axis='x', scilimits=(0,0))

# # if config['title'] == "True":
# # 	if config['transfer_deltaV_cutoff'] == 0:
# # 	 	plt.title("Porkchop plot of TLE elements " + str(a) + " to " + str(b) + 				  \
# # 	 			  ".\n Number of datapoints: " + str(datapoints) + " (" + 						  \
# # 	 			  str( len( departure_epochs ) ) + "x" + str(len(times_of_flight)) + 			  \
# # 	 			  "). Total failures: " + str(failugodres),		 								  \
# # 	 			  fontsize=10, y=1.02)
# # 	else:
# # 		plt.title("Porkchop plot of TLE elements " + str(a) + " to " + str(b) +				 	  \
# # 				  "\n with a cutoff $\Delta V$ of " + str(config['transfer_deltaV_cutoff']) + 	  \
# # 				  "\n Number of datapoints: " + str(datapoints) + " (" + 						  \
# # 				  str(len(departure_epochs)) + "x" + str(len(times_of_flight)) + ") " +			  \
# # 				  "Total failures: " + str(failures), 		  									  \
# # 				  fontsize=10, y=1.02)

# plt.tight_layout()
# plt.savefig(config["output_directory"] + "/" + config["scan_figure"] + "total.png", 					  \
#             dpi=config["figure_dpi"])
# plt.savefig( config['output_directory']+ "goed2.png",             \
#             dpi=config["figure_dpi"])

# print ""
# print "Figure generated successfully!"










































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
