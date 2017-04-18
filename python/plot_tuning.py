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
    # database = sqlite3.connect("/home/enne/work/d2d/data/pagmo/tuning/tuning_all.db")
    database = sqlite3.connect("/home/enne/work/d2d/data/pagmo/tuning/third/tuning_all.db")
    # database = sqlite3.connect("/home/enne/work/d2d/data/pagmo/test_multi.db")


except sqlite3.Error, e:
    print "Error %s:" % e.args[0]
    sys.exit(1)

# combos =  pd.read_sql("SELECT departure_object_id, arrival_object_id from pagmo_scanner_results_4 group by departure_object_id, arrival_object_id", database)

# print combos

best = pd.read_sql("SELECT *,min(total_delta_v) from pagmo_scanner_results_4 LIMIT 1",database)
# print best
lowestDV = best['total_delta_v'][0]
threshold = 1.1*lowestDV
# threshold = 0.4
print threshold


allen = pd.read_sql("   SELECT *,min(total_delta_v) \
                        from pagmo_scanner_results_4 \
                        WHERE total_delta_v < " + str(threshold) + " \
                        GROUP BY population_size,f_variable,cr_variable,run_number,strategy"
                        ,database)
# print allen

# subset = allen.loc[allen['population_size'] == 130].loc[allen['strategy']==1].loc[allen['cr_variable']==0.2].loc[allen['f_variable']==0.2].loc[allen['total_delta_v']<0.4]
columns = ['population_size','strategy','cr_variable','f_variable','length']
save  = pd.DataFrame(columns=columns)
# print len(subset.index)
for strategy in xrange(1,11):
    # print strategy
    for f in [0.2,0.4,0.6,0.8,1.0]:
        # print f
        for cr in [0.2,0.4,0.6,0.8,1.0]:
            for pop in [130,169,260]:
                subset = allen.loc[allen['population_size'] == pop].loc[allen['strategy']==strategy].loc[allen['cr_variable']==cr].loc[allen['f_variable']==f]                
                # print len(subset.index)
                save.loc[len(save)]=[pop,strategy,cr,f,len(subset.index)] 

# print save.sort_values(by='length')
x1 = np.linspace(0.0, 5.0)
x2 = np.linspace(0.0, 2.0)

y1 = np.cos(2 * np.pi * x1) * np.exp(-x1)
y2 = np.cos(2 * np.pi * x2)
p=0
for pop in [130,169,260]:
    fig, ax = plt.subplots(2, 5, figsize=(5,2))  
    x=1
    plt.rc('axes', linewidth=.2)
    colrbr=save.loc[save['population_size']==pop]['length'].max()
    for ax in ax.flatten():  # flatten in case you have a second row at some point            
        sub=save.loc[save['strategy']==x].loc[save['population_size']==pop]    
        plt.subplot(2,5,x)  
        
        # Scatter points
        if sub['length'].max()==colrbr:
            im = plt.scatter(sub['cr_variable'],sub['f_variable'],c=(sub['length']),cmap='jet',vmin=0,vmax=20)
        else:
            plt.scatter(sub['cr_variable'],sub['f_variable'],c=(sub['length']),cmap='jet',vmin=0,vmax=20)
        # Put number of succes in points
        for k in xrange(0,25):
            if int(sub['length'][k*3+(x-1)*75+p])<10:
                plt.annotate(str(int(sub['length'][k*3+(x-1)*75+p])), (float(sub['cr_variable'][k*3+(x-1)*75+p])-0.02,float(sub['f_variable'][k*3+(x-1)*75+p])-0.02),size=3)
            else:
                plt.annotate(str(int(sub['length'][k*3+(x-1)*75+p])), (float(sub['cr_variable'][k*3+(x-1)*75+p])-0.035,float(sub['f_variable'][k*3+(x-1)*75+p])-0.02),size=3)
        # 
        if x in [1,2,3,4,5]:
            plt.tick_params(
                            axis='x',          # changes apply to the x-axis
                            which='both',      # both major and minor ticks are affected
                            bottom='off',      # ticks along the bottom edge are off
                            top='off',         # ticks along the top edge are off
                            labelbottom='off') # labels along the bottom edge are off
        else:
            plt.xlabel("CR variable",size=5,labelpad=.2)

        if not x in [1,6]:
            plt.tick_params(
                            axis='y',          # changes apply to the x-axis
                            which='both',      # both major and minor ticks are affected
                            left='off',      # ticks along the bottom edge are off
                            right='off',         # ticks along the top edge are off
                            labelleft='off') # labels along the bottom edge are off
        else:
            plt.ylabel("F variable",size=5)
            
        
        plt.axis([0,1,0,1])
        plt.axis('equal')
        

        plt.tick_params(axis='both', which='major', labelsize=2.5, width=.2,direction='in',left='off',bottom='off')
        plt.tick_params(axis='both', which='minor', labelsize=2.5, width=.2,direction='in',left='off',bottom='off')
        
        if x==1:
            plt.title("Strategy " + str(x),size=5,y=0.9)
        else:
            plt.title( str(x),size=5,y=0.9)
        x = x+1

    fig.subplots_adjust(right=0.9)
    fig.subplots_adjust(left=0.05)

    cbar_ax = fig.add_axes([0.91, 0.15, 0.02, 0.7])
    fig.colorbar(im, cax=cbar_ax)
    cbar_ax.tick_params(labelsize=6) 
    plt.title("Tuning for threshold: " + str(threshold) + " km/s", size=8,x=-5, y=1.08)
    # plt.legend()    
    plt.savefig("/home/enne/work/d2d/data/pagmo/tuning/third/tuning_" + str(pop) + ".png",dpi=900)
    # plt.savefig("/home/enne/work/d2d/data/pagmo/tuning/figures/tuning_strat_"+ str(strategy) +"_pop_" + str(pop) + "_f_" + str(f) + "_cr_" + str(cr) + ".png", dpi=300)
    plt.close()
    p= p+1



# plt.title("Population size: " + str(pop) + ". Strategy: " + str(strategy) + ". F: " + str(f) + ". CR: " + str(cr) + ". Lowest $\Delta$V: " + str(data['total_delta_v'].min().round(3)) + " km/s" )




# # print allen.sort_values(by='total_delta_v')
# print "Mean: ", allen['total_delta_v'].mean()
# print "Min: ", allen['total_delta_v'].min()

# allen['evalutations']=allen['population_size']*allen['generation']
# # print allen.loc[allen['total_delta_v'] < 1.05*lowestDV][['evalutations','total_delta_v']].sort_values(by='total_delta_v')
# # print allen.loc[allen['total_delta_v'] < 1.05*lowestDV].sort_values(by='total_delta_v')
# # print allen.loc[allen['total_delta_v'] < 0.025+lowestDV][['generation', 'population_size','f_variable','cr_variable', 'evalutations','total_delta_v']].sort_values(by='total_delta_v')
# # print allen.loc[allen['total_delta_v'] < 0.025+lowestDV][['generation', 'population_size','f_variable','cr_variable', 'evalutations','total_delta_v']].sort_values(by='evalutations')
# subgroup = allen.loc[allen['total_delta_v'] < 0.025+lowestDV][['generation', 'population_size','f_variable','cr_variable', 'evalutations','total_delta_v']]
# print subgroup
# subgroup3 = subgroup.groupby(['f_variable'])['cr_variable'].size().nlargest(5).reset_index(name='top5')
# # print subgroup3
# subgroup2 = subgroup.groupby(['cr_variable'])['f_variable'].size().nlargest(5).reset_index(name='top5')
# # print subgroup2
sys.exit()
for strategy in xrange(1,11):
    print strategy
    for f in [0.2,0.4,0.6,0.8,1.0]:
        print f
        for cr in [0.2,0.4,0.6,0.8]:
            for pop in [130,169,260]:
                data = pd.read_sql("SELECT run_number,generation,total_delta_v                            \
                                    FROM pagmo_scanner_results_4  \
                                    WHERE strategy = "+str(strategy)+"\
                                    and f_variable = " +str(f) +"\
                                    and cr_variable = " +str(cr) +"\
                                    and population_size = " +str(pop) ,
                                    database )
                if data.empty==False:
                    fig, ax = plt.subplots()
                    color=iter(cm.rainbow(np.linspace(0,1,19)))
                    for y in xrange(1,19):
                        c=next(color)
                        current_run = data.loc[data['run_number'] == y].sort_values(by='generation')
                        # print current_run
                        plt.step(current_run['generation'][:-2]*pop,current_run['total_delta_v'][:-2],c=c,where='post',lw=1)

                    plt.plot([0,5000*260],[lowestDV,lowestDV],lw=0.5,color='k')
                    plt.plot([5000*130,5000*130],[0,10],lw=0.5,color='k')
                    plt.plot([5000*169,5000*169],[0,10],lw=0.5,color='k')
                    plt.plot([5000*260,5000*260],[0,10],lw=0.5,color='k')

                    plt.axis([0,5000*260,0,2])
                    plt.tight_layout()
                    plt.draw()
                    labelsy = [ w.get_text() for w in ax.get_yticklabels()]
                    labelsy+=[str(lowestDV.round(3))]
                    locsy=list(ax.get_yticks())
                    locsy+=[lowestDV]
                    ax.set_yticklabels(labelsy)
                    ax.set_yticks(locsy)
                    # labelsx = [ w.get_text() for w in ax.get_xticklabels()]
                    labelsx = ["250,000 \n"+str(250000/pop),"500,000 \n"+str(500000/pop),"1,000,000 \n"+str(10000000/pop),"1,250,000 \n"+str(1250000/pop)]
                    labelsx+=["650,000 \n (5000*130)"]
                    labelsx+=["845,000 \n (5000*169)"]
                    locsx = [250000,500000,1000000,1250000]
                    # locsx=list(ax.get_xticks())
                    locsx+=[5000*130]
                    locsx+=[5000*169]
                    ax.set_xlabel("CR [-]")
                    ax.set_ylabel("F [-]")

                    ax.set_xticklabels(labelsx,size=8)
                    ax.set_xticks(locsx)
                    ax.grid(lw=.1)
                    plt.draw()
                    currentDLV = data['total_delta_v'].min()
                    
                    
                    plt.title("Population size: " + str(pop) + ". Strategy: " + str(strategy) + ". F: " + str(f) + ". CR: " + str(cr) + ". Lowest $\Delta$V: " + str(data['total_delta_v'].min().round(3)) + " km/s" )
                    plt.tight_layout()
                    plt.savefig("/home/enne/work/d2d/data/pagmo/tuning/figures/tuning_strat_"+ str(strategy) +"_pop_" + str(pop) + "_f_" + str(f) + "_cr_" + str(cr) + ".png", dpi=300)
                    plt.close()
    
