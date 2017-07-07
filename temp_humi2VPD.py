#!/usr/bin/python
# -*- coding: utf-8 -*-

import codecs
import math

temps = []
humis = []
vpds  = []

'''
 Sonntag近似式を使って飽和水蒸気圧[Pa]を求めます
'''
def temp2svp( temp ):
    temp = temp+273.15
    a = -6096.9385 / temp
    b = 16.635794
    c = -2.711193 / 100 * temp
    d = 1.673952 / 100000 * temp * temp
    e = 2.433502 * math.log(temp)
    return( math.exp( a + b + c + d + e ) * 100 )


with codecs.open( 'ColorMap/html_20170529.txt', 'r', 'utf-8' ) as f:
    for line in f:
        line = line.strip()
        comment = line.replace(' ','').split('#')
        if comment[0]:
            data = comment[0].split('\t')
            temp = float(data[2])
            humi = float(data[3])
            temps.append(temp)
            humis.append(humi)
            svp = temp2svp(temp) # Saturated Vapor Pressure [Pa]
            vp = svp * humi / 100 # Vapor Pressure [Pa]
            vpds.append( (svp-vp)/1000 ) # Vapour Pressure Dificit [kPa]

for vpd in vpds:
    print(str(vpd))




