import matplotlib.pyplot as plt
import csv
import pandas as pd
import sqlite3
import sys
from matplotlib.pyplot import cm 
import numpy as np
# , 'rb'
# with open('../data/thesis/sequences/sequences_2days_0stay.csv') as csvfile:
#      spamreader = csv.reader(csvfile, delimiter=' ', quotechar='|')
#      for row in spamreader:
#          print ', '.join(row)
# if len(sys.argv) != 2:
#     raise Exception("Only provide a JSON config file as input!")

# json_data = open(sys.argv[1])
# config = commentjson.load(json_data)
# json_data.close()
# pprint(config)

print ""
print "******************************************************************"
print "                            Operations                            "
print "******************************************************************"
print ""

print "Fetching porkchop plot data from database ..."

# Connect to SQLite database.
try:
    database = sqlite3.connect("/home/enne/work/d2d/data/pagmo/test2.db")

except sqlite3.Error, e:
    print "Error %s:" % e.args[0]
    sys.exit(1)

combos =  pd.read_sql("SELECT departure_object_id, arrival_object_id from pagmo_scanner_results group by departure_object_id, arrival_object_id", database)

print combos



# test= pd.read_csv('/home/enne/work/d2d/data/pagmo/example.csv', sep=',')

print "test"
for i in range(10,90,10):

    test = pd.read_sql("SELECT *                            \
                            FROM pagmo_scanner_results\
                            WHERE population_size=" + str(i) , 
                            database )
    generations = test['number_of_generations'].max()-1

    # for x in xrange(1,test[''].max()):
    for x in xrange(0,len(combos)):
    # for x in xrange(10,12):
        departure_object_id =   combos['departure_object_id'][x]
        arrival_object_id =     combos['arrival_object_id'][x]

        color=iter(cm.rainbow(np.linspace(0,1,10)))
        newold = test.loc[test['departure_object_id'] == departure_object_id]
        new = newold.loc[ newold['arrival_object_id']== arrival_object_id ]
        # print new

        for y in xrange(1,10):
        # for y in xrange(1,2):
            c=next(color)
            newnew = new.loc[new['run_number'] == y]
            # print newnew
            cmap = plt.get_cmap('hot')
            plt.scatter(newnew['departure_epoch'],newnew['time_of_flight'],c=newnew['transfer_delta_v'],s=5)
        plt.title(str(departure_object_id) + " to " + str(arrival_object_id))

        plt.axis([0,14*86400,0,2*86400])
        plt.tight_layout()

        plt.savefig("/home/enne/work/d2d/data/pagmo/porkchop/"+ str(departure_object_id) +"_"+str(arrival_object_id)+"_porkchop_pop_"+str(i)+".png", dpi=300)
        plt.close()
    
# print test['total_delta_v'],1/test['removed_area']


# try:
#     database = sqlite3.connect("../data/temp/test_lambert_scanner_zoom.db")

# except sqlite3.Error, e:
#     print "Error %s:" % e.args[0]
#     sys.exit(1)

# # if config['objects']==[]:
# #   best = pd.read_sql("SELECT DISTINCT(departure_object_id), arrival_object_id, time_of_flight,  \
# #                                       departure_epoch, transfer_delta_v                         \
# #                       FROM lambert_scanner_zoom_results                                             \
# #                       ORDER BY transfer_delta_v ASC                                             \
# #                       LIMIT 10 ",                                                               \
# #                       database )

# #   a = (best['departure_object_id'][0])
# #   b = (best['arrival_object_id'][0])
# # else:
# #   a = config['objects'][0]
# #   b = config['objects'][1]

# # print "Porkchop plot figure being generated for transfer between TLE objects", a, "and", b, "..."

# raw_data = pd.read_sql_query("  SELECT arrival_object_id, departure_object_id  \
#                                 FROM lambert_scanner_zoom_results  \
#                                 GROUP BY departure_object_id, arrival_object_id  \
#                                 ORDER BY transfer_delta_v ASC \
#                                 LIMIT 10",\
#                                         database)

# raw_data = pd.read_sql_query("select name from sqlite_master where type = 'table';",\
#                                         database)

# print raw_data
# sys.exit()
# print 2








# def plot_pareto_frontier(Xs, Ys, color='b', filename='output.png', maxX=True, maxY=True):
#     '''Pareto frontier selection process'''
#     sorted_list = sorted([[Xs[i], Ys[i]] for i in range(len(Xs))], reverse=maxY)
#     pareto_front = [sorted_list[0]]
#     for pair in sorted_list[1:]:
#         if maxY:
#             if pair[1] > pareto_front[-1][1]:
#                 pareto_front.append(pair)
#         else:
#             if pair[1] < pareto_front[-1][1]:
#                 pareto_front.append(pair)
    
#     '''Plotting process'''
#     plt.scatter(Xs,Ys,color=color)
#     pf_X = [pair[0] for pair in pareto_front]
#     pf_Y = [pair[1] for pair in pareto_front]
#     plt.plot(pf_X, pf_Y)
#     print pf_X,pf_Y
#     # plt.scatter(pf_X,pf_Y,color='r')
#     plt.title("Pareto front of 4 (red), 5 (blue), 6 (black), and 7 (green) debris sequences.")
#     plt.xlabel("Total sequence delta V [km/s]")
#     plt.ylabel("Removed area from orbit [m^2]")
#     plt.xlim(0,3)
#     plt.ylim(-70,0)
#     plt.rc('grid', linestyle="-", color='black')
#     # plt.grid(True)
#     # plt.show()
#     # plt.tight_layout()
#     plt.savefig(filename, dpi=300)


# pareto_4 = pd.read_csv('../data/thesis/sequences/pareto_4.csv', sep=' ')
# pareto_5 = pd.read_csv('../data/thesis/sequences/pareto_5.csv', sep=' ')
# pareto_6 = pd.read_csv('../data/thesis/sequences/pareto_6.csv', sep=' ')
# pareto_7 = pd.read_csv('../data/thesis/sequences/pareto_7.csv', sep=' ')
# plot_pareto_frontier(pareto_4['total_delta_v'],-pareto_4['removed_area'],color='r',maxY=False)
# plot_pareto_frontier(pareto_5['total_delta_v'],-pareto_5['removed_area'],color='b',maxY=False)
# plot_pareto_frontier(pareto_6['total_delta_v'],-pareto_6['removed_area'],color='k',maxY=False)
# plot_pareto_frontier(pareto_7['total_delta_v'],-pareto_7['removed_area'],color='g',maxY=False)
# # plot_pareto_frontier(test['total_delta_v'],test['removed_area'])

