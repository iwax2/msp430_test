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
import codecs
from datetime import datetime
from collections import deque
from time import sleep

NO_NODES = 3
NODE_LIST_FILE = 'node_list.txt'

received_packets = deque() # パケット保存用キュー
accepted_packets = [] # 受理したパケット保存用リスト
coordinate = {} # 各ノードの座標保管用ディクショナリ (16進4桁ノードID, x座標, y座標)

s = serial.Serial('/dev/ttymxc1', 115200, timeout=1)
packet_number = 0

'''
 パケットを受け取ることに専念するスレッド
 ERXDATA_0000_0000_0000_0000_00_00_00000_0000_0000
         FROM DEST MID  SEL  RS LEN DATA RUT1 RUT2
'''
def receive_packet(e):
    while not e.isSet():
        s.flush()
        if( s.inWaiting() > 0 ):
            line = s.readline().decode('utf-8').rstrip()
            print("    Received: "+ line)
            if( line.startswith('E') ):
                data = line.split()
                if( len(data)>=8 and data[0] == 'ERXDATA' ):
                    send_id = data[1]
                    t_and_h = data[7].split(',')
                    packet_number = int(t_and_h[0]) # packet_id
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
    with codecs.open( NODE_LIST_FILE, 'r', 'utf-8' ) as f:
        for line in f:
            line = line.strip()
            comment = line.replace(' ','').split('#')
            if comment[0]:
                data = comment[0].split(',')
                coordinate[data[0]] = (data[1], data[2])
                print(data)
    accepted_node_list = []
    start_sec = datetime.now().second
    while( len(accepted_packets) < len(coordinate) ):
        if( len(received_packets)>0 ):
            packet = received_packets.popleft()
            if( packet[1] == packet_number ):
                accepted_packets.append(packet)
                accepted_node_list.append(packet[0])
                print("Packet is captured from : " + packet[0])
            else:
                print("[Information] lost packet : (" + packet[0] +","+ str(packet[1]) +","+ packet[2] +","+ packet[3] +") / Current PacketID: "+str(packet_number))
        else: # パケットが届いている間は再送要求を送らない
            if( datetime.now().second - start_sec > 10 ):
                start_sec = datetime.now().second # 前回の実行から10秒以上経っていれば再送要求する
                for node in coordinate:
                    if node not in accepted_node_list:
                        s.flush()
                        cmd = "SKSEND 1 1000 0002 0C RSEND,"+node+","+str(packet_number)+"\r\n"
                        print(cmd)
                        s.write(cmd.encode('utf-8'))
                        sleep(2)
                        s.flush()
                        print(cmd)
                        s.write(cmd.encode('utf-8'))

'''
 メインスレッド
'''
e = threading.Event()
th_rec = threading.Thread(name='rec', target=receive_packet, args=(e,))
th_rec.start()
try:
    while True:
        th_arq = threading.Thread(name='arq', target=automatic_repeat_request, args=(packet_number,))
        th_arq.start()
        th_arq.join(30) # 30秒で打ち切る
        packet_number = ( packet_number+1 ) % 10
        sleep_time = 60 - datetime.now().second
        cmd = "SKSEND 1 1000 0002 0F SLEEP_ALL_,"+str(packet_number)+","+str(sleep_time)+"\r\n"
        s.flush()
        s.write(cmd.encode('utf-8'))
        print(cmd)
        sleep(5)
        s.flush()
        s.write(cmd.encode('utf-8'))
        print(cmd)
        date = datetime.now().strftime("%Y/%m/%d,%H:%M:%S")
        for packet in accepted_packets:
            csv = date+','+','+packet[2]+','+packet[3]
            with open('temperature_'+ packet[0] +'.csv','a') as f:
                f.write(csv+'\r\n')
        accepted_packets.clear()
        print("[Information] Sleep....")
        sleep(sleep_time)
        print("[Information] Wakeup!")

except KeyboardInterrupt:
    print("W: interrupt received, stopping script")
finally:
    print("W: close...")
    s.close()
e.set()

th_rec.join()

sys.exit()


