import pandas as pd
import numpy as np
import sys
# print sys.argv[1]
if len(sys.argv)<4:
	print 'Please provide 3 strings, the path to TLE.txt, the path to SatCat.txt, and the output path.'
	sys.exit()

# dataTLE = pd.read_fwf('../data/Kartik/LEO_TLEs_thesis_Enne.txt', colspecs=[(2,7)], header=None)
dataTLE = pd.read_fwf(sys.argv[1], colspecs=[(2,7)], header=None)
dataTLE =  dataTLE[1::3]
listOfTLEs=map(int,dataTLE[0].values.tolist())

#using pandas with a column specification
col_specification =[(0, 12), (13, 18), (19, 20), (20, 21), (21, 22), (23, 47), (49, 54), (56, 66), (68, 73), (75, 85), (87, 94), (96, 101), (103, 109), (111, 117), (119, 127),(129,132)]
# path = '../data/satcat.txt'
data = pd.read_fwf(sys.argv[2], colspecs=col_specification, header=None, names=['NORAD Catalog Number','International Designator','Multiple Name Flag','Payload Flag ("*" if payload, blank otherwise)','Operational status code','Satellite name','Source of Ownership','Launch Date [year-month-day]','Launch site','Decay Date, if applicable [year-month-day]','Orbital period [minutes]','Inclination [degrees]','Apogee Altitude [kilometers]','Perigee Altitude [kilometers]','Radar Cross Section [meters2] (N/A if no data available)','Orbital status code'])
data = data[data['International Designator'].isin(listOfTLEs)]
data = data.reset_index()

data.loc[data['Operational status code'] == '+', 'Operational Status'] = 'Operational'
data.loc[data['Operational status code'] == 'D', 'Operational Status'] = 'Decayed'
data.loc[data['Operational status code'] == '-', 'Operational Status'] = 'Nonoperational'
data.loc[data['Operational status code'] == 'P', 'Operational Status'] = 'Partially Operational'
data.loc[data['Operational status code'] == 'B', 'Operational Status'] = 'Backup/Standby'
data.loc[data['Operational status code'] == 'S', 'Operational Status'] = 'Spare'
data.loc[data['Operational status code'] == 'X', 'Operational Status'] = 'Extended Mission'
data.loc[data['Operational status code'] == '?', 'Operational Status'] = 'Unknown'

data.loc[data['Orbital status code'] == '?'  , 'Orbital Status'] = 'Unknown'
data.loc[data['Orbital status code'] == 'NCE', 'Orbital Status'] = 'No Current Elements'
data.loc[data['Orbital status code'] == 'AS1', 'Orbital Status'] = 'Asteriod Landing'
data.loc[data['Orbital status code'] == 'DOC', 'Orbital Status'] = 'Permanently Docked'
data.loc[data['Orbital status code'] == 'EA2', 'Orbital Status'] = 'Earth Impact'
data.loc[data['Orbital status code'] == 'EL0', 'Orbital Status'] = 'Earth Lagrange Orbit'
data.loc[data['Orbital status code'] == 'EM0', 'Orbital Status'] = 'Earth Moon Barycenter Orbit'
data.loc[data['Orbital status code'] == 'ISS', 'Orbital Status'] = 'Docked to International Space Station'
data.loc[data['Orbital status code'] == 'JU0', 'Orbital Status'] = 'Jupiter Orbit'
data.loc[data['Orbital status code'] == 'JU2', 'Orbital Status'] = 'Jupiter Impact'
data.loc[data['Orbital status code'] == 'MA0', 'Orbital Status'] = 'Mars Orbit'
data.loc[data['Orbital status code'] == 'MA1', 'Orbital Status'] = 'Mars Landing'
data.loc[data['Orbital status code'] == 'MA2', 'Orbital Status'] = 'Mars Impact'
data.loc[data['Orbital status code'] == 'MO0', 'Orbital Status'] = 'Moon Orbit'
data.loc[data['Orbital status code'] == 'MO1', 'Orbital Status'] = 'Moon Landing'
data.loc[data['Orbital status code'] == 'MO2', 'Orbital Status'] = 'Moon Impact'
data.loc[data['Orbital status code'] == 'MO3', 'Orbital Status'] = 'Moon Roundtrip'
data.loc[data['Orbital status code'] == 'NEA', 'Orbital Status'] = 'No Elements Available'
data.loc[data['Orbital status code'] == 'NIE', 'Orbital Status'] = 'No Initial Elements'
data.loc[data['Orbital status code'] == 'SA0', 'Orbital Status'] = 'Saturn Oribit'
data.loc[data['Orbital status code'] == 'SS0', 'Orbital Status'] = 'Solar System Escape Orbit'
data.loc[data['Orbital status code'] == 'SU0', 'Orbital Status'] = 'Solar Orbit'
data.loc[data['Orbital status code'] == 'VE0', 'Orbital Status'] = 'Venus Orbit'
data.loc[data['Orbital status code'] == 'VE1', 'Orbital Status'] = 'Venus Landing'
data.loc[data['Orbital status code'] == 'VE2', 'Orbital Status'] = 'Venus Impact'


data.to_csv(sys.argv[3])

print "Succesfully constructed",sys.argv[3]," containing",len(data),"satellites from",sys.argv[1],"and additional data from",sys.argv[2],'.'
# data.head(n=500000).to_csv('outputenne.csv')

