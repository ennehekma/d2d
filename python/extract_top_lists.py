'''
Copyright (c) 2014-2016 Kartik Kumar, Dinamica Srl (me@kartikkumar.com)
Copyright (c) 2016 Enne Hekma, Delft University of Technology (ennehekma@protonmail.com)
Distributed under the MIT License.
See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
'''

# Set up modules and packages
# Plotting
# import matplotlib
# matplotlib.use('Agg')
# import matplotlib.pyplot as plt
# from matplotlib import gridspec
# from matplotlib import cm
# from mpl_toolkits.mplot3d import Axes3D
# from mpl_toolkits.mplot3d import axes3d
# from nlcmap import nlcmap

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
print "      Copyright (c) 2016, E.J. Hekma (ennehekma@protonmail.com)   "
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

print "Fetching scan data from database ..."

# for i in xrange(0,len(config['database'])):
for i in xrange(0,1):
    print "Computations being done on database:", config['database_tag'][i]
    print config['database'][i]
    try:
        database = sqlite3.connect(config['database'][i])

    except sqlite3.Error, e:

        print "Error %s:" % e.args[0]
        sys.exit(1)

    toplist = pd.read_sql("SELECT * FROM atom_scanner_results LIMIT 10", database)
    # toplist.colums = list(toplist)
    # toplist.columns = ['atom_transfer_id', 'lambert_transfer_id', 'atom_departure_delta_v_x', 'atom_departure_delta_v_y', 'atom_departure_delta_v_z', 'atom_arrival_delta_v_x', 'atom_arrival_delta_v_y', 'atom_arrival_delta_v_z', 'atom_transfer_delta_v']
    # print toplist['atom_transfer_id']
    # print toplist.columns


    toplist_atom = pd.read_sql("SELECT *, min(atom_scanner_results.atom_transfer_delta_v)                                                          \
                           FROM lambert_scanner_results                                           \
                           INNER JOIN    atom_scanner_results                                     \
                           ON lambert_scanner_results.transfer_id                                 \
                              = atom_scanner_results.lambert_transfer_id                          \
                           GROUP BY lambert_scanner_results.departure_object_id,                  \
                                    lambert_scanner_results.arrival_object_id                     \
                           ORDER BY atom_scanner_results.atom_transfer_delta_v                    \
                                    ASC                                                           \
                           LIMIT 10000000                                                         \
                           ;",                                                                    \
                           database)
    toplist_atom2 = pd.concat([ toplist_atom['transfer_id'],
                                # toplist_atom['lambert_transfer_id'],
                                toplist_atom['departure_object_id'],
                                toplist_atom['arrival_object_id'],
                                toplist_atom['departure_epoch'],
                                toplist_atom['time_of_flight'],
                                toplist_atom['revolutions'],
                                # toplist_atom['time_of_flight'],
                                toplist_atom['transfer_delta_v'],
                                toplist_atom['atom_transfer_delta_v'] ], axis=1)

    # toplist_lambert = pd.read_sql("SELECT *, min(lambert_scanner_results.arrival_delta_v_x)       \
    toplist_lambert = pd.read_sql("SELECT *, min(lambert_scanner_results.transfer_delta_v)        \
                           FROM lambert_scanner_results                                           \
                           INNER JOIN    atom_scanner_results                                     \
                           ON lambert_scanner_results.transfer_id                                 \
                              = atom_scanner_results.lambert_transfer_id                          \
                           GROUP BY lambert_scanner_results.departure_object_id,                  \
                                    lambert_scanner_results.arrival_object_id                     \
                           ORDER BY lambert_scanner_results.transfer_delta_v                      \
                           # ORDER BY lambert_scanner_results.arrival_delta_v_x                     \
                                    ASC                                                           \
                           LIMIT 10000000                                                         \
                           ;",                                                                    \
                           database)
    toplist_lambert2 = pd.concat([ toplist_lambert['transfer_id'],
                                   # toplist_lambert['atom_transfer_id'],
                                   toplist_lambert['departure_object_id'],
                                   toplist_lambert['arrival_object_id'],
                                   toplist_lambert['departure_epoch'],
                                   toplist_lambert['time_of_flight'],
                                   toplist_lambert['revolutions'],
                                   # toplist_lambert['time_of_flight'],
                                   toplist_lambert['transfer_delta_v'],
                                   toplist_lambert['atom_transfer_delta_v'] ], axis=1)

    toplist_atom2['atom_dv_ranking'] = toplist_atom2['atom_transfer_delta_v'].rank('min')
    toplist_lambert2['lambert_dv_ranking'] = toplist_lambert2['transfer_delta_v'].rank('min')
    toplist_atom2['combo'] = toplist_atom2['departure_object_id'].map(str) + '-' + toplist_atom2['arrival_object_id'].map(str)
    toplist_lambert2['combo'] = toplist_lambert2['departure_object_id'].map(str) + '-' + toplist_lambert2['arrival_object_id'].map(str)

    toplist_lambert2.join(toplist_atom2, on='combo', how='left', lsuffix="_atom")
    test =  pd.merge(toplist_atom2,toplist_lambert2, on='combo', how='outer')
    # test['lambert_dv_ranking'] = test['transfer_delta_v'].rank('min')
    # test['atom_dv_ranking'] = test['atom_transfer_delta_v'].rank('min')
    # print 'toplist_atom2'
    # print toplist_atom2
    # print 'toplist_lambert2'
    # print toplist_lambert2
    # print 'test'
    # print test
    test.to_csv(str(config['output_directory'] + config['database_tag'][i] + ".csv"), index=False )

# SELECT * FROM atom_scanner_results INNER JOIN lambert_scanner_results ON lambert_scanner_results.transfer_id = atom_scanner_results.lambert_transfer_id WHERE lambert_scanner_results.transfer_id=14441079;
# Connect to SQLite database.
# try:
#     database = sqlite3.connect(config['database'])

# except sqlite3.Error, e:

#     print "Error %s:" % e.args[0]
#     sys.exit(1)


# departure_epochs = pd.read_sql("SELECT DISTINCT departure_epoch                                   \
#                                     FROM lambert_scanner_results;",                               \
#                                 database)
# for i in xrange(0,departure_epochs.size):
#     c = departure_epochs['departure_epoch'][i]
#     print "Plotting scan map with departure epoch: ",c,"Julian Date"

#     # Fetch scan data.
#     map_order = "departure_" + config['map_order']
#     scan_data = pd.read_sql("SELECT departure_object_id, arrival_object_id,                       \
#                                     min(transfer_delta_v), "+ map_order + "                       \
#                                 FROM lambert_scanner_results                                      \
#                                 WHERE departure_epoch BETWEEN " + str(c-0.00001) + "              \
#                                                       AND "+str(c+0.00001) +"                     \
#                                 GROUP BY departure_object_id, arrival_object_id;",                \
#                             database)
#     scan_data.columns = ['departure_object_id','arrival_object_id',                               \
#                          'transfer_delta_v',str(map_order)]
#     scan_order = scan_data.sort_values(str(map_order))                                            \
#                           .drop_duplicates('departure_object_id')[                                \
#                               ['departure_object_id',str(map_order)]]

#     scan_map = scan_data.pivot(index='departure_object_id',                                       \
#                                columns='arrival_object_id',
#                                values='transfer_delta_v')
#     scan_map = scan_map.reindex(index=scan_order['departure_object_id'],                          \
#                                 columns=scan_order['departure_object_id'])

#     # Set up color map.
#     bins = np.linspace(scan_data['transfer_delta_v'].min(),                                       \
#                        scan_data['transfer_delta_v'].max(), 10)
#     groups = scan_data['transfer_delta_v'].groupby(                                               \
#                 np.digitize(scan_data['transfer_delta_v'], bins))
#     levels = groups.mean().values
#     cmap_lin = plt.get_cmap(config['colormap'])
#     cmap = nlcmap(cmap_lin, levels)

#     # Plot heat map.
#     ax1 = plt.subplot2grid((15,15), (2, 0),rowspan=13,colspan=14)
#     heatmap = ax1.pcolormesh(scan_map.values, cmap=cmap,                                          \
#                              vmin=scan_data['transfer_delta_v'].min(),                            \
#                              vmax=scan_data['transfer_delta_v'].max())
#     ax1.set_xticks(np.arange(scan_map.shape[1] + 1)+0.5)
#     ax1.set_xticklabels(scan_map.columns, rotation=90)
#     ax1.set_yticks([])
#     ax1.tick_params(axis='both', which='major', labelsize=config['tick_label_size'])
#     ax1.set_xlim(0, scan_map.shape[1])
#     ax1.set_ylim(0, scan_map.shape[0])
#     ax1.set_xlabel('Departure object',fontsize=config['axis_label_size'])
#     ax1.set_ylabel('Arrival object',fontsize=config['axis_label_size'])

#     # Plot axis ordering.
#     ax2 = plt.subplot2grid((15,15), (0, 0),rowspan=2,colspan=14,sharex=ax1)
#     ax2.step(np.arange(0.5,scan_map.shape[1]+.5),scan_order[str(map_order)],'k',linewidth=2.0)
#     ax2.get_yaxis().set_major_formatter(plt.FormatStrFormatter('%.2e'))
#     ax2.tick_params(axis='both', which='major', labelsize=config['tick_label_size'])
#     plt.setp(ax2.get_xticklabels(), visible=False)
#     ax2.set_ylabel(config['map_order_axis_label'],fontsize=config['axis_label_size'])

#     # Plot color bar.
#     ax3 = plt.subplot2grid((15,15), (0, 14), rowspan=15)
#     color_bar = plt.colorbar(heatmap,cax=ax3)
#     color_bar.ax.get_yaxis().labelpad = 20
#     color_bar.ax.set_ylabel(r'Total transfer $\Delta V$ [km/s]', rotation=270)

#     plt.tight_layout()

#     # Save figure to file.
#     plt.savefig(config["output_directory"] + "/" + config["scan_figure"] + "_"+str(i+1) +         \
#                        ".png", dpi=config["figure_dpi"])
#     plt.close()
#     print "Figure ",i+1," generated successfully...."

print "Figure generated successfully!"
print ""

# Close SQLite database if it's still open.
# if database:
#     database.close()

# Stop timer
end_time = time.time( )

# Print elapsed time
print "Script time: " + str("{:,g}".format(end_time - start_time)) + "s"

print ""
print "------------------------------------------------------------------"
print "                         Exited successfully!                     "
print "------------------------------------------------------------------"
print ""
