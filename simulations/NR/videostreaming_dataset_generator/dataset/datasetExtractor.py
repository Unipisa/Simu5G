import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import sys
#%matplotlib inline

pd.set_option('display.width', 180)
pd.set_option('display.max_colwidth', 100)
pd.options.mode.chained_assignment = None  # default='warn'

def parse_if_number(s):
    try: return float(s)
    except: return True if s=="true" else False if s=="false" else s if s else None

def parse_ndarray(s):
    return  np.fromstring(s, sep=' ') if s else None

def isolateNode(name):
    l = name.split(".")
    if("mecHost" in l[1]):
        return l[2]
    else:
        return l[1]

def renameStatistic(name):
    stat = name.split(":")[0]
    if stat == "rtpPakcetSize":
        return "rtpPacketSize"
    else:
        return stat
    
def renameNetParams(name):
    name = name.replace("#2a" ,"*")
    return name.replace("probCqi=false," ,"")

def toString(arr):
    return ' '.join(map(str, arr))

def cleanRow(row):
    row['module'] = isolateNode(row['module'])
    row['statistic'] = renameStatistic(row['statistic'])
    return row

fileName = sys.argv[1]

allValues = pd.read_csv(fileName, converters = {
    'attrvalue': parse_if_number,
    'attrname': parse_if_number,
    'vectime': parse_ndarray,
    'vecvalue': parse_ndarray})

# get only vector values
vectors = allValues[allValues.type=='vector']
# vector statistic of interest
dataset = vectors
# delete unnecessary columns
dataset.drop(columns=['type', 'attrname', 'attrvalue'], axis = 1, inplace=True)
# # rename
dataset.rename(columns={'vectime': 'timestamps', 'vecvalue': 'values', 'name': 'statistic'}, inplace=True)

# get network parameters
parameters = allValues[allValues.attrname=='iterationvarsf']
parameters = parameters[['run', 'attrvalue']]
parameters.rename(columns={'attrvalue': 'network_parameters'}, inplace=True)
newDataset = dataset.merge(parameters, on='run')
# shift column 'Name' to first position
first_column = newDataset.pop('network_parameters')

# newDataset['values']=newDataset['values'].map(list)
# newDataset['timestamps']=newDataset['timestamps'].map(list)

newDataset['values']=newDataset['values'].apply(toString)
newDataset['timestamps']=newDataset['timestamps'].apply(toString)
#newDataset.apply(lambda row : cleanRow(row), axis = 1)
newDataset['module'] = newDataset['module'].apply(isolateNode)
# newDataset['statistic'] = newDataset['statistic'].apply(renameStatistic)
# newDataset['network_parameters'] = newDataset['network_parameters'].apply(renameNetParams)

newfile = fileName.split(".csv")[0]+"_dataset.csv"
newDataset.to_csv(newfile, index = False, decimal = ".")
# dataset.to_csv("dataset.csv", index = False, decimal = ".")
print("done")
