#from tensorflow import keras
#import numpy as np
#import matplotlib.pyplot as plt
from numpy import genfromtxt
import iso8601
import pytz
import json
import matplotlib.pyplot as plt

def print_hi(name):
    #data = genfromtxt('pressure rize.csv', delimiter=',', skip_header=4)
    #print(data)
    newCsv = {}
    filename = "pressure rize.csv"
    in_file = open(filename, "r")#2022-07-06_18 36_Sand
    for counter in range(0, 4):
        in_line = in_file.readline()
    while True:
        in_line = in_file.readline()
        if not in_line:
            break
        #print(in_line)
        values = in_line.split(",")
        if len(values) < 8:
            continue
        if values[5] == "_time" or values[5] == "dateTime:RFC3339" or values[5] == "false" or values[5] == "":
            continue
        _date_obj = iso8601.parse_date(values[5])
        _date_utc = _date_obj.astimezone(pytz.utc)
        timestamp = _date_utc.timestamp() * 1000
        value = values[6]
        einheit = values[7]
        if timestamp in newCsv.keys():
            newCsv[timestamp][einheit] = value
        else:
            newCsv[timestamp] = {einheit : value}
    in_file.close()
    f = open(filename[:-4] + ".json", "a")
    f.write(json.dumps(newCsv))
    f.close()
    print(newCsv)
    """
    plt.figure()
    tims = []
    pv = []
    ph = []
    pb = []
    """
    tempstart = 0
    standtime = []
    standstart = False
    for timestamp in newCsv.keys():
        if (standstart and int(newCsv[timestamp]["pH"])<2550 and int(newCsv[timestamp]["pV"])<2550):
            standtime.append(timestamp - tempstart)
            standstart = False
        if (not standstart and (int(newCsv[timestamp]["pH"])>2550 or int(newCsv[timestamp]["pV"])>2550)):
            tempstart = timestamp
            standstart = True
    print(standtime)
    print(sum(standtime)/len(standtime))
    """
    plt.plot(tims, pv)
    plt.plot(tims, ph)
    plt.plot(tims, pb)
    plt.show()
    plt.close()
"""
# Press the green button in the gutter to run the script.
if __name__ == '__main__':
    print_hi('PyCharm')

# See PyCharm help at https://www.jetbrains.com/help/pycharm/
