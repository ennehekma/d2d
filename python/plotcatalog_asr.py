'''
Copyright (c) 2014, K. Kumar (me@kartikkumar.com)
All rights reserved.
'''

###################################################################################################
# Set up input deck
###################################################################################################

# Set path to TLE catalog file.
tleCatalogFilePathall		= "../data/catalog/3le.txt"
tleCatalogFilePathSSO		= "../data/SSO/SSO_tle.txt"
tleCatalogFilePathGEO		= "../data/GEO/GEO_tle.txt"
tleCatalogFilePathHEO		= "../data/HEO/HEO_tle.txt"

# Set number of lines per entry in TLE catalog (2 or 3).
tleEntryNumberOfLines		= 3

# Set path to output directory.
outputPath 					= "../data/SSO/plots/"

# Set figure DPI.
figureDPI 					= 300

# Set font size for axes labels.
fontSize 					= 22

###################################################################################################

'''
						DO NOT EDIT PARAMETERS BEYOND THIS POINT!!!
'''

###################################################################################################
# Set up modules and packages
###################################################################################################

from sgp4.earth_gravity import wgs72
from sgp4.io import twoline2rv
from sgp4.propagation import getgravconst

from matplotlib import rcParams
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from twoBodyMethods import convertMeanMotionToSemiMajorAxis

###################################################################################################

###################################################################################################
# Read and store TLE catalog
###################################################################################################


# years = [2013, 2014, 2015]
# dn = []
# for year in years:
#     df1 = pd.DataFrame({'Incidents': [ 'C', 'B','A'],
#                  year: [1, 1, 1 ],
#                 }).set_index('Incidents')
#     dn.append(df1)
# dn = pd.concat(dn, axis=1)
# print(dn)

incl5 = []
raan3 = []
ecc3 = []
ecc5 = []
eccentricity = []
sma = []
inclinations3 = []
aop3 = []
order = ['all','SSO','GEO','HEO']
markers = ['.','s','+','D']
colors = ['k','b','g','r']
for x in order:
	# print eval(str('tleCatalogFilePath' + order[x]))
	# print x
	# time.sleep(10)
	tleCatalogFilePathNew = eval(str('tleCatalogFilePath' + str(x)))
	# print tleCatalogFilePathNew
	# Read in catalog and store lines in list.
	fileHandle = open(tleCatalogFilePathNew)
	catalogLines = fileHandle.readlines()
	fileHandle.close()

	# Strip newline and return carriage characters.
	for i in xrange(len(catalogLines)):
	    catalogLines[i] = catalogLines[i].strip('\r\n')

	# Parse TLE entries and store debris objects.
	debrisObjects = []
	for tleEntry in xrange(0,len(catalogLines),tleEntryNumberOfLines):
		debrisObjects.append(twoline2rv(catalogLines[tleEntry+1], catalogLines[tleEntry+2], wgs72))

	# Sort list of debris objects based on inclination.
	inclinationSortedObjects = sorted(debrisObjects, key=lambda x: x.inclo, reverse=False)

	inclinations = []
	raan = []
	ecc = []
	aop = []
	for i in xrange(len(inclinationSortedObjects)):
		inclinations.append(inclinationSortedObjects[i].inclo)
		raan.append(inclinationSortedObjects[i].nodeo)
		ecc.append(inclinationSortedObjects[i].ecco)
		aop.append(inclinationSortedObjects[i].argpo)
	
	smatemp = []
	smatemp = [convertMeanMotionToSemiMajorAxis(debrisObject.no/60.0, getgravconst('wgs72')[1])-6373 \
		  for debrisObject in debrisObjects]
	smatemp2 = []
	smatemp2 = pd.DataFrame(smatemp, columns=[str(x)])
	sma.append(smatemp2)
	
	ecctemp = []
	ecctemp = [debrisObject.ecco for debrisObject in debrisObjects]
	ecc4 = []
	ecc4 = pd.DataFrame(ecctemp, columns=[str(x)])
	ecc5.append(ecc4)
	
	incltemp = []
	incltemp = [np.rad2deg(debrisObject.inclo) for debrisObject in debrisObjects]
	incl4 = []
	incl4 = pd.DataFrame(incltemp, columns=[str(x)])
	incl5.append(incl4)

	raan2 = []
	raan2 = pd.DataFrame(raan, columns=[str(x)])
	raan3.append(raan2)
	
	ecc2 = []
	ecc2 = pd.DataFrame(ecc, columns=[str(x)])
	ecc3.append(ecc2)

	aop2 = []
	aop2 = pd.DataFrame(aop, columns=[str(x)])
	aop3.append(aop2)

	inclinations2 = []
	inclinations2 = pd.DataFrame(inclinations, columns=[str(x)])
	inclinations3.append(inclinations2)
# print 'exit'
sma = pd.concat(sma, axis=1)
ecc5 = pd.concat(ecc5, axis=1)
incl5 = pd.concat(incl5, axis=1)

raan3 = pd.concat(raan3, axis=1)
ecc3 = pd.concat(ecc3, axis=1)
aop3 = pd.concat(aop3, axis=1)
inclinations3 = pd.concat(inclinations3, axis=1)
# print raan3['GEO']

	# print str('inclinations' + order[x] + 'new' )
	# str('inclinations' + order[x] + 'new' ) = []
	# str('inclinations' + order[x] + 'new' ) = inclinations
	# str('raan' + order[x] + 'new' ) = []
	# str('raan' + order[x] + 'new' ) = raan
	# str('ecc' + order[x] + 'new' ) = []
	# str('ecc' + order[x] + 'new' ) = ecc
	# str('aop' + order[x] + 'new' ) = []
	# str('aop' + order[x] + 'new' ) = aop
	# print raan[1]
# print raanSSOnew[1]
###################################################################################################

###################################################################################################
# Generate plots
###################################################################################################

# Set font size for plot labels.
rcParams.update({'font.size': fontSize})
markers = ['.','s','+','D']
colors = ['k','b','g','r']

# Plot distribution of eccentricity [-] against semi-major axis [km].
figure = plt.figure()
axis = figure.add_subplot(111)
plt.xlabel("Semi-major axis altitude [km]")
plt.ylabel("Eccentricity [-]")
plt.ticklabel_format(style='sci', axis='x', scilimits=(0,0))
# print 
# print inclinationSortedObjects[1].no
plt.plot([convertMeanMotionToSemiMajorAxis(debrisObject.no/60.0, getgravconst('wgs72')[1]) \
		  for debrisObject in debrisObjects], \
	     [debrisObject.ecco for debrisObject in debrisObjects], \
	     marker='o', markersize=1, color='k', linestyle='none')
# axis.set_xlim(xmax=0.8e4)
figure.set_tight_layout(True)
plt.savefig(outputPath + "/figure1_debrisPopulation_eccentricityVsSemiMajorAxis.pdf", \
			dpi = figureDPI)
plt.close()

figure = plt.figure()
axis = figure.add_subplot(111)
plt.xlabel("Semi-major axis altitude [km]")
plt.ylabel("Eccentricity [-]")
plt.ticklabel_format(style='sci', axis='x', scilimits=(0,0))
plt.plot(sma['all'],ecc3['all'], \
			     marker='.', markersize=1, color='k', linestyle='none')
plt.plot(sma['SSO'],ecc3['SSO'], \
			     marker='s', markersize=10, color='c', linestyle='none')
plt.plot(sma['GEO'],ecc3['GEO'], \
			     marker='^', markersize=10, color='g', linestyle='none')
plt.plot(sma['HEO'],ecc3['HEO'], \
			     marker='D', markersize=6, color='r', linestyle='none')
axis.set_xlim(xmax=0.5e5)
figure.set_tight_layout(True)
plt.savefig(outputPath + "/figure1_debrisPopulation_eccentricityVsSemiMajorAxisNew.pdf", \
			dpi = figureDPI)
plt.close()

# Plot components of eccentricity vector [-].
figure = plt.figure()
axis = figure.add_subplot(111)
plt.xlabel("$e \cos{\omega}$ [-]")
plt.ylabel("$e \sin{\omega}$ [-]")
plt.plot(ecc*np.cos(aop),ecc*np.sin(aop), marker='o', markersize=1, color='k', linestyle='none')
plt.axis('equal')
axis.set_xlim(xmin=-.21, xmax=.21)
axis.set_ylim(ymin=-.21, ymax=.21)
axis.set(xticks=[-.2,-.1,0,.1,.2])#, xticklabels=datelabels) #Same as plt.xticks
figure.set_tight_layout(True)
plt.savefig(outputPath + "/figure2_debrisPopulation_eccentricityVector.pdf", dpi = figureDPI)
# plt.savefig(outputPath + "/figure2_debrisPopulation_eccentricityVector.pdf", dpi = figureDPI)
plt.close()

figure = plt.figure()
axis = figure.add_subplot(111)
plt.xlabel("$e \cos{\omega}$ [-]")
plt.ylabel("$e \sin{\omega}$ [-]")
# plt.plot(ecc*np.cos(aop),ecc*np.sin(aop), marker='o', markersize=1, color='k', linestyle='none')
plt.plot(ecc3['all']*np.cos(aop3['all']),ecc3['all']*np.sin(aop3['all']), marker='.', markersize=1, color='k', linestyle='none')
plt.plot(ecc3['SSO']*np.cos(aop3['SSO']),ecc3['SSO']*np.sin(aop3['SSO']), marker='s', markersize=10, color='c', linestyle='none')
plt.plot(ecc3['GEO']*np.cos(aop3['GEO']),ecc3['GEO']*np.sin(aop3['GEO']), marker='^', markersize=10, color='g', linestyle='none')
plt.plot(ecc3['HEO']*np.cos(aop3['HEO']),ecc3['HEO']*np.sin(aop3['HEO']), marker='D', markersize=6, color='r', linestyle='none')
plt.axis('equal')
# axis.set_xlim(xmin=-.21, xmax=.21)
# axis.set_ylim(ymin=-.21, ymax=.21)
# axis.set(xticks=[-.2,-.1,0,.1,.2])#, xticklabels=datelabels) #Same as plt.xticks
figure.set_tight_layout(True)
plt.savefig(outputPath + "/figure2_debrisPopulation_eccentricityVectorNew.pdf", dpi = figureDPI)
# plt.savefig(outputPath + "/figure2_debrisPopulation_eccentricityVector.pdf", dpi = figureDPI)
plt.close()


# Plot distribution of inclination [deg] against semi-major axis [km].
figure = plt.figure()
axis = figure.add_subplot(111)
plt.xlabel("Semi-major axis altitude [km]")
plt.ylabel("Inclination [deg]")
plt.ticklabel_format(style='sci', axis='x', scilimits=(0,0))
plt.plot(sma['all'],incl5['all'], \
			     marker='.', markersize=1, color='k', linestyle='none')
plt.plot(sma['SSO'],incl5['SSO'], \
			     marker='s', markersize=10, color='c', linestyle='none')
plt.plot(sma['GEO'],incl5['GEO'], \
			     marker='^', markersize=10, color='g', linestyle='none')
plt.plot(sma['HEO'],incl5['HEO'], \
			     marker='D', markersize=6, color='r', linestyle='none')
# plt.plot([convertMeanMotionToSemiMajorAxis(debrisObject.no/60.0, getgravconst('wgs72')[1]) \
# 		  for debrisObject in debrisObjects], \
# 	     [np.rad2deg(debrisObject.inclo) for debrisObject in debrisObjects], \
# 	     marker='o', markersize=1, color='k', linestyle='none')
axis.set_xlim(xmax=0.5e5)
figure.set_tight_layout(True)
plt.savefig(outputPath + "/figure3_debrisPopulation_inclinationVsSemiMajorAxisNew.pdf", \
			dpi = figureDPI)
plt.close()

# Plot components of inclination vector [deg].
figure = plt.figure()
axis = figure.add_subplot(111)
plt.xlabel("$i \cos{\Omega}$ [deg]")
plt.ylabel("$i \sin{\Omega}$ [deg]")
plt.plot(np.rad2deg(inclinations)*np.cos(raan),np.rad2deg(inclinations)*np.sin(raan), \
		 marker='o', markersize=1, color='k', linestyle='none')
plt.axis('equal')
axis.set_xlim(xmin=-180.0, xmax=180.0)
axis.set_ylim(ymin=-180.0, ymax=180.0)
figure.set_tight_layout(True)
plt.savefig(outputPath + "/figure4_debrisPopulation_inclinationVector.pdf", dpi = figureDPI)
plt.close()

# Plot components of inclination vector [deg].
figure = plt.figure()
axis = figure.add_subplot(111)
plt.xlabel("$i \cos{\Omega}$ [deg]")
plt.ylabel("$i \sin{\Omega}$ [deg]")
# plt.plot(np.rad2deg(inclinations)*np.cos(raan),np.rad2deg(inclinations)*np.sin(raan), \
# 		 marker='o', markersize=1, color='k', linestyle='none')
plt.plot(np.rad2deg(inclinations3['all'])*np.cos(raan3['all']),np.rad2deg(inclinations3['all'])*np.sin(raan3['all']), \
		 marker='.', markersize=1, color='k', linestyle='none')
plt.plot(np.rad2deg(inclinations3['SSO'])*np.cos(raan3['SSO']),np.rad2deg(inclinations3['SSO'])*np.sin(raan3['SSO']), \
		 marker='s', markersize=10, color='c', linestyle='none')
plt.plot(np.rad2deg(inclinations3['GEO'])*np.cos(raan3['GEO']),np.rad2deg(inclinations3['GEO'])*np.sin(raan3['GEO']), \
		 marker='^', markersize=10, color='g', linestyle='none')
plt.plot(np.rad2deg(inclinations3['HEO'])*np.cos(raan3['HEO']),np.rad2deg(inclinations3['HEO'])*np.sin(raan3['HEO']), \
		 marker='D', markersize=6, color='r', linestyle='none')
plt.axis('equal')
axis.set_xlim(xmin=-180.0, xmax=180.0)
axis.set_ylim(ymin=-180.0, ymax=180.0)
figure.set_tight_layout(True)
plt.savefig(outputPath + "/figure4_debrisPopulation_inclinationVectorNew.pdf", dpi = figureDPI)
plt.close()




###################################################################################################
###################################################################################################