#!/usr/bin/python
# -*- coding: utf-8 -*-

# Armadillo上でLPR9204から送られてきたデータを受信してCSV保存するスクリプト
import serial
import sys
from datetime import datetime

f = open('temperature.csv', 'w')
s = serial.Serial('/dev/ttymxc1', 115200, timeout=1)

try:
    while True:
        line = s.readline().decode('utf-8')
        if( line.startswith('E') ):
            data = line.split()
            if( len(data)>=8 and data[0] == 'ERXDATA' ):
                date = datetime.now().strftime("%Y/%m/%d,%H:%M:%S")
                dest = data[1]
                t_h = data[7].split(',')
                temp = t_h[0]
                humi = t_h[1]
                csv = date+','+dest+','+temp+','+humi
                print(csv)
                f.write(csv+'\r\n')
except KeyboardInterrupt:
    print("W: interrupt received, stopping script")
finally:
    print("W: close...")
    s.close()
    f.close()

sys.exit()


