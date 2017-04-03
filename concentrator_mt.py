#!/usr/bin/python
# -*- coding: utf-8 -*-

'''
 Armadillo上のLPR9204コンセントレータ（マルチスレッド制御）
 
 各ノードから送られてきたデータを受信して、
 足らなければ再送要求して、
 データがそろったら全ノードを60-かかった秒だけ寝かせる
'''
import serial
import sys
import threading
from datetime import datetime
from collections import deque
from time import sleep

NO_NODES = 3
NODE_LIST_FILE = 'node_list.txt'

received_packets = deque() # パケット保存用キュー
accepted_packets = [] # 受理したパケット保存用リスト
coordinate = {} # 各ノードの座標保管用ディクショナリ (16進4桁ノードID, x座標, y座標)

s = serial.Serial('/dev/ttymxc1', 115200, timeout=1)

'''
 パケットを受け取ることに専念するスレッド
 ERXDATA_0000_0000_0000_0000_00_00_00000_0000_0000
         FROM DEST MID  SEL  RS LEN DATA RUT1 RUT2
'''
def receive_packet(e):
    while not e.isSet():
        line = s.readline().decode('utf-8')
        if( line.startswith('E') ):
            data = line.split()
            if( len(data)>=8 and data[0] == 'ERXDATA' ):
                send_id = data[1]
                t_and_h = data[7].split(',')
                packet_number = t_and_h[0] # packet_id
                temp    = t_and_h[1]
                humi    = t_and_h[2]
                packet  = (send_id, packet_number, temp, humi)
                received_packets.append(packet)

'''
 受け取ったパケットを処理する(ARQ)スレッド
 @args packet_number パケット番号が異なるものは破棄する
 1. 全ノードからデータが送られてきているかをチェック
 2. 送られてきていないノードに対しては再送要求
 3. データが揃えばスレッド終了
'''
def automatic_repeat_request(packet_number):
    coordinate.clear()
    with open( NODE_LIST_FILE, 'r' ) as f:
        for line in f:
            comment = line.replace(' ','').split('#')
            if not comment[0]:
                data = comment[0].split(',')
                coordinate[data[0]] = (data[1], data[2])
    accepted_node_list = []
    start_sec = datetime.now().second
    while( len(accepted_packets) < len(coordinate) ):
        if( len(received_packets)>0 ):
            packet = received_packets.popleft()
            if( packet[1] == packet_number ):
                accepted_packets.push(packet)
                accepted_node_list.append(packet[0])
                print("Packet is captured from : " + packet[0])
            else:
                print "[Information] lost packet : " + packet
        else: # パケットが届いている間は再送要求を送らない
            if( datetime.now().second - start_sec > 0 ):
                start_sec = datetime.now().second # 前回の実行から1秒以上経っていれば再送要求する
                for node in coordinate:
                    if node not in accepted_node_list
                        print("SKSEND 1 1000 0002 B RSEND,"+node+","+str(packet_number))
                        s.println("SKSEND 1 1000 0002 B RSEND,"+node+","+str(packet_number))

'''
 メインスレッド
'''
e = threading.Event()
th_rec = threading.Thread(name='rec', target=receive_packet, args(e,))
th_rec.start()
try:
    packet_number = 0
    while True:
        th_arq = threading.Thread(name='arq', target=automatic_repeat_request, args(packet_number,))
        th_arq.start()
        th_arq.join(10)
        packet_number = ( packet_number+1 ) % 10
        sleep_time = 60 - datetime.now().second
        s.println("SKSEND 1 1000 0002 B SLEEP_ALL0,"+str(packet_number)+","+str(sleep_time))
        date    = datetime.now().strftime("%Y/%m/%d,%H:%M:%S")
        for packet in accepted_packets:
            csv = date+','+','+packet[2]+','+packet[3]
            with open('temperature_'+ packet[0] +'.csv','a') as f:
                f.write(csv+'\r\n')
        while( len(received_packets) <= 0 ):# データが送られてくるまで待機
            sleep(1)
except KeyboardInterrupt:
    print("W: interrupt received, stopping script")
finally:
    print("W: close...")
    s.close()
e.set()
th_rec.join()

sys.exit()


