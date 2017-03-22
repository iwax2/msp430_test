#!/usr/bin/python
# -*- coding: utf-8 -*-

# Armadillo上でLPR9204から送られてきたデータを受信してCSV保存するスクリプト
import serial
import sys
from datetime import datetime

s = serial.Serial('/dev/ttymxc1', 115200, timeout=1)

try:
    while True:
        line = s.readline().decode('utf-8')
        if( line.startswith('E') ):
            data = line.split()
            if( len(data)>=8 and data[0] == 'ERXDATA' ):
                date    = datetime.now().strftime("%Y/%m/%d,%H:%M:%S")
                send_id = data[1]
                t_and_h = data[7].split(',')
                temp    = t_and_h[0]
                humi    = t_and_h[1]
                csv     = date+','+ send_id +','+temp+','+humi
                print(csv)
                with open('temperature_'+ send_id +'.csv','a') as f:
                    f.write(csv+'\r\n')
except KeyboardInterrupt:
    print("W: interrupt received, stopping script")
finally:
    print("W: close...")
    s.close()

sys.exit()


