
#!/usr/bin/env python3
import numpy as np
import pandas as pd
import seaborn as sns
from fitter import Fitter, get_common_distributions, get_distributions
import matplotlib.pyplot as plt
from scipy.stats import chi


f = open("car_data", "w")
# write first line (fps)
f.write("30\t[frames/second]\tFrame Rate\n")
# write second line
f.write("0\t[seconds]\tInitial Delay\n")
firstLine = True
Istatus = []
Bstatus = []
Pstatus = []
with open("h264_car") as file:
    for line in file:
        if(firstLine):
            firstLine = False
        else:
            # print("\n")
            f.write("\n")
        frame, frameType, size = line.rstrip().split("\t")
        if(frameType == "IDR"):
            frameType = "I-Frame"
            Istatus.append(int(size))     
        elif(frameType == "B"):
            frameType = "B-Frame"
            Bstatus.append(int(size))  
        elif(frameType == "P"):
            frameType = "P-Frame"
            Pstatus.append(int(size))  

        size = int(size)
        # print(str((size))+"\t[bits]\t"+frameType)
        f.write(str(size)+"\t[bits]\t"+frameType)
    
    f.close()

# sns.set_style('white')
# sns.set_context("paper", font_scale = 2)
# sns.displot(Istatus)
print(max(Istatus))
print(min(Istatus))
print(max(Bstatus))
print(min(Bstatus))
print(max(Pstatus))
print(min(Pstatus))

# size, scale = 1000, 10
# commutes = pd.Series(Bstatus)

# commutes.plot.hist(grid=True, bins=10, rwidth=0.9,
#                    color='#607c8e')
# plt.title('Commute Times for 1,000 Commuters')
# plt.xlabel('Counts')
# plt.ylabel('Commute Time')
# plt.grid(axis='y', alpha=0.75)

# plt.show()
# mu, sigma = 0, 0.1 # mean and standard deviation
# data = np.random.normal(mu, sigma, 10000)

# f = Fitter(Bstatus,
#            distributions=get_distributions())
# f.fit()
# print(f.summary())

# print(f.get_best(method = 'sumsquare_error'))
# # print(f.get_best(method = 'sumsquare_error'))