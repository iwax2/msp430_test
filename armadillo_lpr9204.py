#!/usr/bin/python
# -*- coding: utf-8 -*-

# Armadillo上でLPR9204から送られてきたデータを受信してCSV保存するスクリプト
import serial
import sys

f = open('temperature.csv', 'w')
with serial.Serial('/dev/ttymxc1', 115200, timeout=1) as s:

try:
    while True:
        line = s.readline()   # read a '\n' terminated line
        f.write(line)
except KeyboardInterrupt:
    print("W: interrupt received, stopping script")
finally:
    print("W: close…")
    s.close()
    f.close()

sys.exit()


