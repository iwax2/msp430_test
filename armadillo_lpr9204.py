#!/usr/bin/python
# -*- coding: utf-8 -*-

# Armadillo上でLPR9204から送られてきたデータを受信してCSV保存するスクリプト
import serial
import sys
import codecs
from datetime import datetime
from time import sleep

NODE_LIST_FILE = 'node_list.txt'

#s = serial.Serial('/dev/ttymxc1', 115200, timeout=1)
#s = serial.Serial('/dev/tty.usbserial-AL00MUQ0', 115200, timeout=1)
s = serial.Serial('COM7', 115200, timeout=1)

coordinate = {}

def read_serial():
    while True:
        line = s.readline().decode('utf-8')
        if( line.startswith('E') ):
            data = line.split()
            if( len(data)>=8 and data[0] == 'ERXDATA' ):
                t_and_h = data[7].split(',') # n,temp,humi
                if( len(t_and_h)>=3 ):
                    date    = datetime.now().strftime("%Y/%m/%d,%H:%M:%S")
                    send_id = data[1]
                    temp    = t_and_h[1]
                    humi    = t_and_h[2]
                    csv     = date+','+ send_id +','+temp+','+humi
                    print(csv)
#                    with open('temperature_'+ send_id +'.csv','a') as f:
                    with open('korokoro_20170909.csv','a') as f:
                        f.write(csv+'\r\n')

def write_serial():
    packet_number = 0
    while True:
        packet_number = ( packet_number + 1 ) % 10
        cmd = "SKSEND 1 1000 0002 0E SLEEP_ALL0,"+str(packet_number)+",30\r\n"
        print(cmd)
        s.write(cmd.encode('utf-8'))
        sleep(5)

def read_settings():
    with codecs.open( NODE_LIST_FILE, 'r', 'utf-8' ) as f:
        for line in f:
            line = line.strip()
            print(line)
            comment = line.replace(' ','').split('#')
            if comment[0]:
                data = comment[0].split(',')
                coordinate[data[0]] = (data[1], data[2])
                print(data)
    sleep(5)


try:
    read_serial()
    #write_serial()
    #read_settings()
except KeyboardInterrupt:
    print("W: interrupt received, stopping script")
finally:
    print("W: close...")
    s.close()

sys.exit()

