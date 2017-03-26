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

NO_NODES = 3

received_packets = deque() # パケット保存用リスト
accepted_packets = [] # 受理したパケット保存用リスト

#s = serial.Serial('/dev/ttymxc1', 115200, timeout=1)

# パケットを受け取ることに専念するスレッド
def receive_packet(e):
    while not e.isSet():
#        line = s.readline().decode('utf-8')
        line = input('920 SAYS> ')
        if( line.startswith('E') ):
            data = line.split()
            if( len(data)>=8 and data[0] == 'ERXDATA' ):
                send_id = data[1]
                t_and_h = data[7].split(',')
                packet_number = t_and_h[0]
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
    start_sec = datetime.now().second
    while( len(accepted_packets) < NO_NODES ):
        if( len(received_packets)>0 ):
            packet = received_packets.popleft()
            if( packet[1] == packet_number ):
                accepted_packets.push(packet)
            else:
                print "[Information] lost packet : " + packet
        else:
            # パケットが届いている間は再送要求を送らない
            if( datetime.now().second - start_sec > 0 ):
                # 前回の実行から1秒以上経っていれば再送要求する
                s.println("SKSEND 1 1000 ") # 足りないIDが分からないな、どうしよう
            


e = threading.Event()
th_rec = threading.Thread(name='rec', target=receive_packet, args(e,))
th_rec.start()
try:
    packet_number = 0
    while True:
        th_arq = threading.Thread(name='arq', target=automatic_repeat_request, args(packet_number,))
        packet_number = ( packet_number+1 ) % 10
        th_arq.start()
        th_arq.join(10)
        # SLEEP命令を発効する（終わった or タイムアウトした）
        # ここでファイルに書き込みを行う
#                date    = datetime.now().strftime("%Y/%m/%d,%H:%M:%S")
#                with open('temperature_'+ send_id +'.csv','a') as f:
#                    f.write(csv+'\r\n')
        del packets[:]
        # データが送られてくるまで待機
except KeyboardInterrupt:
    print("W: interrupt received, stopping script")
finally:
    print("W: close...")
    s.close()
e.set()
th_rec.join()

sys.exit()


