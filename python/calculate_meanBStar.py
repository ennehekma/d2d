'''
Copyright (c) 2014-2015, Kartik Kumar (me@kartikkumar.com)
All rights reserved.
'''

# Set up modules and packages
# Plotting
import matplotlib
# matplotlib.use('TkAgg')
import matplotlib.pyplot as plt
# from matplotlib import gridspec
# from matplotlib import cm
# from mpl_toolkits.mplot3d import Axes3D
# from mpl_toolkits.mplot3d import axes3d
# from nlcmap import nlcmap

# # I/O
# import commentjson
# import json
# from pprint import pprint
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

print ""
print "******************************************************************"
print "                            Operations                            "
print "******************************************************************"
print ""

print "Fetching scan data from database ..."

# Connect to SQLite database.
try:
    database = sqlite3.connect('/usr/local/d2d/data/meanBStar/meanBStartest.sqlite')

except sqlite3.Error, e:

    print "Error %s:" % e.args[0]
    sys.exit(1)

# Fetch scan data.
scan_data = pd.read_sql("SELECT departure_object_id, arrival_object_id   \
                            FROM lambert_scanner_results                                        \
                            LIMIT 50000;",                  \
                        database)
z = map(float,np.array(scan_data['arrival_object_id']))
y = np.arange(0, 752, dtype=np.float)

print np.size(y)
print np.mean(np.abs(z)), np.size(z), np.std(z), np.max(z), np.min(z), np.min(np.abs(z))

# plt.scatter(y,np.abs(z))
plt.scatter(y,z)
plt.axhline(np.mean(np.abs(z)),linewidth=2,color='r',label="Mean of absolute values of BStar: "+str(np.mean(np.abs(z)))) 
plt.axhline(np.mean(z),linewidth=2,color='b',label="Mean of values of BStar: " +str(np.mean(z)))
plt.axis([0, 753, -0.00065, np.max(z)])
plt.title("BStar statistics: Max: "+str(np.max(z))+" Min: "+str(np.min(z))+" Standard deviation: "+str(np.std(z))   +"\n One data point not on this scale: the minimum of "+str(np.min(z)))
plt.xlabel("TLE object (LEO<1500 km)")
plt.ylabel("BStar ")
plt.legend(loc='lower left')
plt.grid()
plt.show()

# Stop timer
end_time = time.time( )

# Print elapsed time
print "Script time: " + str("{:,g}".format(end_time - start_time)) + "s"

print ""
print "------------------------------------------------------------------"
print "                         Exited successfully!                     "
print "------------------------------------------------------------------"
print ""