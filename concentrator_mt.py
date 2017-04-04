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
ack_message_queue = deque() # EACKメッセージ保存用キュー
message_id_queue = deque()  # 920が割り振るメッセージID保存用キュー
error_code_queue = deque()  # FAIL ER10などのエラーコード保存用キュー

s = serial.Serial('/dev/ttymxc1', 115200, timeout=1)
packet_number = 0

'''
 パケットを受け取ることに専念するスレッド
 ERXDATA_0000_0000_0000_0000_00_00_00000_0000_0000
         FROM DEST MID  SEL  RS LEN DATA RUT1 RUT2
'''
def receive_packet(e):
    while not e.isSet():
        if( s.inWaiting() > 0 ):
            line = s.readline().decode('utf-8').rstrip()
            print("    920 Says>"+ line)
            data = line.split()
            if( len(data)>=8 and data[0] == 'ERXDATA' ):
                send_id = data[1]
                t_and_h = data[7].split(',')
                packet_number = int(t_and_h[0]) # packet_id
                temp    = t_and_h[1]
                humi    = t_and_h[2]
                packet  = (send_id, packet_number, temp, humi)
                received_packets.append(packet)
            elif( len(data)>=4 and data[0] == 'EACK' ):
                ack_result = ( data[1], data[3] ) # STATUS, MSG_ID
                ack_message_queue.append( ack_result )
            elif( len(data)>=2 and data[1] == 'OK' ):
                message_id_queue.append(data[0])
            elif( len(data)>=2 and data[0] == 'FAIL' ):
                error_code_queue.append(data[1])

'''
 受け取ったパケットを処理する(ARQ)スレッド
 @args packet_number パケット番号が異なるものは破棄する
 1. 全ノードからデータが送られてきているかをチェック
 2. 送られてきていないノードに対しては再送要求
 3. データが揃えばスレッド終了
'''
def automatic_repeat_request(e, packet_number):
    coordinate.clear()
    with codecs.open( NODE_LIST_FILE, 'r', 'utf-8' ) as f:
        for line in f:
            line = line.strip()
            comment = line.replace(' ','').split('#')
            if comment[0]:
                data = comment[0].split(',')
                coordinate[data[0]] = (data[1], data[2])
                print(data)
    accepted_node_list = [] # 対象のパケット番号で受理したパケットの管理用
    start_sec = datetime.now().second
    while( len(accepted_packets) < len(coordinate) and not e.isSet() ):
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
                        send_packet("SKSEND 1 1000 0002 0C RSEND,"+node+","+str(packet_number))

'''
 パケット送信（再送制御つき）
 メソッド側で改行コードは面倒を見ます
'''
def send_packet( command ):
    serial_command = command + "\r\n"
    s.flush()
    ack_message_queue.clear()
    message_id_queue.clear()
    error_code_queue.clear()
    print(command)
    s.write(serial_command.encode('utf-8'))
    message_id = "" # ランダムの4桁16進値。SKSENDを発行したらMSG_ID OKの形で返ってくる（ハズ
    while True:
        if( len(message_id_queue) > 0 ):
            message_id = message_id_queue.popleft()
            break
    while True:
        if( len(ack_message_queue) > 0 ):
            if( pop_ack_message( message_id ) == '1' ):
                break
            else:
                print("[Resend because of  nack] "+command)
                s.flush()
                s.write(serial_command.encode('utf-8'))
        elif( len(error_code_queue) > 0 ): # エラーメッセージがでたときは再送
            print("[Resend because of error] "+command)
            s.flush()
            s.write(serial_command.encode('utf-8'))

'''
 ack_message_queueから対象のmessage_idを含むACK結果を取り出して返します
'''
def pop_ack_message( message_id ):
    for ack in ack_message_queue: # ack[0] = STATUS, ack[1] = MSG_ID
        if( ack[1] == message_id ):
            ack_message_queue.remove(ack)
            return ack[0] # ack_status '0' or '1'
    return '0' # ACKが受け取れていないので'0'を返して再送


'''
 メインスレッド
'''
e_rec = threading.Event()
e_arq = threading.Event()
th_rec = threading.Thread(name='rec', target=receive_packet, args=(e_rec,))
th_rec.start()
try:
    while True:
        e_arq.clear()
        th_arq = threading.Thread(name='arq', target=automatic_repeat_request, args=(e_arq, packet_number))
        th_arq.start()
        th_arq.join(30) # 30秒で打ち切る
        e_arq.set()
        packet_number = ( packet_number+1 ) % 10
        sleep_time = 60 - datetime.now().second
        # SLEEPを送るのはブロードキャストになると思うので別メソッドになる（予定）
        send_packet("SKSEND 1 1000 0002 0F SLEEP_ALL_,"+str(packet_number)+","+str(sleep_time))
        date = datetime.now().strftime("%Y/%m/%d,%H:%M:%S")
        for packet in accepted_packets:
            csv = date+','+','+packet[2]+','+packet[3] # packet = (send_id, packet_number, temp, humi)
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


