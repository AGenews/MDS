#!/usr/bin/env python
# -*- coding: utf-8 -*-

#///////////////////////////////////////////////////////////////////////////////
#/ MIT License                                                                //
#/                                                                            //   
#/ Copyright (c) [2017] [Andreas Genewsky]                                    //  
#/                                                                            //
#/ Permission is hereby granted, free of charge, to any person obtaining a    //
#/ copy of this software and associated documentation files (the "Software"), //
#/ to deal in the Software without restriction, including without limitation  //
#/ the rights to use, copy, modify, merge, publish, distribute, sublicense,   //
#/ and/or sell copies of the Software, and to permit persons to whom the      //
#/ Software is furnished to do so, subject to the following conditions:       //
#/                                                                            //
#/ The above copyright notice and this permission notice shall be included    //
#/ in all copies or substantial portions of the Software.                     //
#/                                                                            //
#/ THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR //
#/ IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   //
#/ FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    //
#/ THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER //
#/ LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    //
#/ FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        //   
#/ DEALINGS IN THE SOFTWARE.                                                  //
#///////////////////////////////////////////////////////////////////////////////

__author__ = 'Andreas Genewsky (2017)'
import argparse
import numpy as np
import matplotlib.mlab as mlab
import matplotlib.pyplot as plt
from datetime import datetime
import math
np.seterr(all='ignore')

parser = argparse.ArgumentParser(description="**** Motion Detector Analysis Script ****",
                                 epilog="<<< Andreas Genewsky (2017) - Max-Planck Institute for Psychiatry >>>")
                                 
parser.add_argument('-i','--input', help='Input file name',required=True)
parser.add_argument('-o','--output',help='Output file name', required=True)
parser.add_argument('-b','--bin',help='Bin Width in Milliseconds', required=True)
args = parser.parse_args()
inputfile = args.input
outputfile = args.output
binwidth = int(args.bin)
data = np.genfromtxt(inputfile, delimiter=',',skip_header=1)
timestamps = []
dtime = []
abstime = []
# MONTH, DAY, YEAR, HH, MM, SS, mmm
for x in range(0,data.shape[0]):
    YYYY = int(data[x,2])
    MM = int(data[x,0])
    DD = int(data[x,1])
    HH = int(data[x,3])
    mm = int(data[x,4])
    SS = int(data[x,5])
    ms = int(data[x,6])*1000
    timestamps.append(datetime(YYYY,MM,DD,HH,mm,SS,ms))

for x in range(0,len(timestamps)):
    abst = ((timestamps[x]-timestamps[0]).total_seconds())*1000
    abstime.append(abst) 

mod = data[:,7:14]
abstime=np.array([abstime], dtype='float64').T
condata = np.concatenate((abstime,mod),axis=1)

maxBin = round(max(abstime[:,0]))
bincount = int(maxBin/(binwidth*1.0))
lastBin = maxBin-(maxBin%bincount)
bincount = int(lastBin/(binwidth*1.0))
bins = np.linspace(0,lastBin+binwidth,bincount+1,dtype='int',endpoint=False)
bins = np.array([bins]).T
bindata = np.zeros((bins.shape[0],condata.shape[1]),dtype='float64')
for x in range(0,bins.shape[0]):
    eventcounter = 0.0
    index = 0
    for y in range(0,condata.shape[0]):
        if( (condata[y,0]>=bins[x,0]) and (condata[y,0]<(bins[x,0]+binwidth)) ):
            bindata[x,:]=bindata[x,:]+condata[y,:]
            eventcounter += 1.0
            index = y
    condata = condata[index:condata.shape[0],:]
    bindata[x,7]=bindata[x,7]/eventcounter
    bindata[x,0]=bins[x,0]  
    print (str( round(((float(bins[x,0])/float(bins.max()))*100),2) )+" %")  
    
np.savetxt(outputfile, bindata, delimiter=',',fmt='%10.3f')
