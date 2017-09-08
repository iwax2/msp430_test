#!/usr/bin/python3 -u
# -*- coding: utf-8 -*-

'''
 Armadillo上のLPR9204コンセントレータ（マルチスレッド制御）

 各ノードから送られてきたデータを受信して、
 足らなければ再送要求して、
 データがそろったら全ノードを60-かかった秒だけ寝かせる
'''
import serial
import sys
import codecs
import math
import os
import threading
import subprocess
import pyfiap
from datetime import datetime
from collections import deque
from time import sleep

NODE_LIST_FILE = '/home/iwata/node_list.txt'
PID_FILE = '/var/run/concentratord.pid'
OUTPUT_FILE = '/home/iwata/temp_humi_'
INTERMITTENT_TIME = 60

serial_readline = deque() # シリアルデータ
received_packets = deque() # パケット保存用キュー
accepted_packets = [] # 受理したパケット保存用リスト
coordinate = {} # 各ノードの座標保管用ディクショナリ (16進4桁ノードID, x座標, y座標)
ack_message_queue = deque() # EACKメッセージ保存用キュー
message_id_queue = deque()  # 920が割り振るメッセージID保存用キュー
error_code_queue = deque()  # FAIL ER10などのエラーコード保存用キュー

s = serial.Serial('/dev/ttymxc1', 115200, timeout=1)
#s = serial.Serial('/dev/tty.usbserial-AL00MUQ0', 115200, timeout=1)
#s = serial.Serial('COM3', 115200, timeout=1)
packet_number = 0

fiap = pyfiap.fiap.APP("http://ants.jga.kisarazu.ac.jp/axis2/services/FIAPStorage?wsdl")

'''
 パケットを受け取ることに専念するスレッド
 ERXDATA_0000_0000_0000_0000_00_00_00000_0000_0000
         FROM DEST MID  SEL  RS LEN DATA RUT1 RUT2
'''
def receive_packet(e, ):
    while not e.isSet():
        if( s.readable() and s.in_waiting > 0 ):
#            data = s.read(s.in_waiting)
#            print("Serial>"+str(data))
#            line = data.decode('utf-8').rstrip()
            line = s.readline().decode('utf-8').rstrip()
            print("920>"+ line)
            serial_readline.append(line)

def routing_packet(e, ):
    while not e.isSet():
        if( len(serial_readline) > 0 ):
            line = serial_readline.popleft();
#            print("920>"+ line)
            data = line.split()
            if( len(data)>=8 and data[0] == 'ERXDATA' ):
                send_id = data[1]
                t_and_h = data[7].split(',')
                p_id   = int(t_and_h[0]) # packet_id
                temp   = t_and_h[1]
                humi   = t_and_h[2]
                packet = (send_id, p_id, temp, humi)
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
    coordinate.clear() # 今回取得すべき座標一覧
    with codecs.open( NODE_LIST_FILE, 'r', 'utf-8' ) as f:
        for line in f:
            line = line.strip()
            comment = line.replace(' ','').split('#')
            if comment[0]:
                data = comment[0].split(',')
                coordinate[data[0]] = (data[1], data[2])
#                print(data)
    start_sec = datetime.now().second
    # 座標情報
    while( len(coordinate) > 0 and not e.isSet() ):
#        print(str(datetime.now().second))
        if( len(received_packets)>0 ):
            packet = received_packets.popleft()
            # 対象のパケットIDでかつ、対象の座標である
            if( packet[1] == packet_number and packet[0] in coordinate ):
                coord = coordinate.pop(packet[0]) # 座標情報の更新
                packet += coord # パケットに座標情報を付与
                accepted_packets.append(packet)
                print("Captured: " + packet[0] + " and Remaining : ", end="" )
                print(coordinate.keys())
#            else:
#                print("[Information] lost packet : (" + packet[0] +","+ str(packet[1]) +","+ packet[2] +","+ packet[3] +") / Current PacketID: "+str(packet_number))
#                print("packet[1] == packet_number", packet[1] == packet_number)
#                print("packet[0] in coordinate", packet[0] in coordinate)
''' 再送制御なし（バグる）
        else: # パケットが届いている間は再送要求を送らない
            if( datetime.now().second - start_sec > 10 ):
                start_sec = datetime.now().second # 前回の実行から5秒以上経っていれば再送要求する
                for node in coordinate:
                    send_packet("SKSEND 1 1000 "+node+" 0C RSEND,"+node+","+str(packet_number))
                    if( e.isSet() ): # 再送制御中にスレッドが残ることがあるので止める
                        break
'''

'''
 寝ているときに受け取ったパケットを処理する(insleep)スレッド
 @args packet_number 次のパケットのデータであれば寝かさず起こしておく
'''
def insleep_automatic_repeat_request(e_slp, packet_number):
    while( e_slp.isSet() ):
        if( len(received_packets)>0 ):
            packet = received_packets.popleft()
            if( packet[1] != packet_number ): # でたらめなパケットIDのデータが届けばsleepさせる
                sleep_time = broadcast_sleep_all(1)
            else: # 次のパケットならもう一回received_packetsに入れてぐーるぐる
                received_packets.append(packet)

'''
 パケット送信（×再送制御）
 IoTノードが寝てるときに再送制御してもずっと寝てるので再送してもあまり意味は無い
 メソッド側で改行コードは面倒を見ます
'''
def send_packet( command ):
    s.flush()
    ack_message_queue.clear()
    message_id_queue.clear()
    error_code_queue.clear()
    sent_message_id = set()
    message_id = _send_packet_and_get_message_id( command )
    sent_message_id.add(message_id)
    number_of_sent_packets = 1
    
    # 一回ぐらいは再送してみる
    while number_of_sent_packets < 2:
        if( len(ack_message_queue) > 0 ):
            ack_message = pop_ack_message( sent_message_id )
            if( ack_message == '1' ):
                break
            elif( len(ack_message_queue) < 1 or ack_message == '0' ): # ackキューが空/ackメッセージが0なら再送
                print("[Resend because of  nack] "+command)
#                sleep(2)
                message_id = _send_packet_and_get_message_id( command )
                sent_message_id.add(message_id)
                number_of_sent_packets += 1
        elif( len(error_code_queue) > 0 ): # エラーメッセージがでたときは再送
            error_message = error_code_queue.popleft()
            print("[Resend because of error] "+command+" / "+error_message)
            message_id = _send_packet_and_get_message_id( command )
            sent_message_id.add(message_id)
            number_of_sent_packets += 1

'''
 sleep_allを発信
'''
def broadcast_sleep_all( no_resend ):
    sleep_time = INTERMITTENT_TIME - datetime.now().second
    str_st = '{0:02d}'.format(sleep_time)
    # SLEEPはブロードキャスト（最大3ホップ）で送信
    broadcast_packet("SKBC 3 1000 0F SLEEP_ALL_,"+str(packet_number)+","+str_st, no_resend)
    return(sleep_time)

'''
 ブロードキャスト送信（再送制御なし）
 メソッド側で改行コードは面倒を見ます
'''
def broadcast_packet( command, no_resend ):
    for i in range(no_resend):
        message_id = _send_packet_and_get_message_id( command )
        if( len(error_code_queue) > 0 ): # エラーメッセージがでたときは再送
            error_message = error_code_queue.popleft()
            print("[Resend because of error] "+command+" / "+error_message)
            message_id = _send_packet_and_get_message_id( command )
        sleep(1)

'''
 パケットを送信してメッセージIDを取得します
'''
def _send_packet_and_get_message_id( command ):
#    print(command)
    serial_command = command + "\r\n"
    s.write(serial_command.encode('utf-8'))
    message_id = "" # ランダムの4桁16進値。SKSENDを発行したらMSG_ID OKの形で返ってくる（ハズ
    while True:
        if( len(message_id_queue) > 0 ):
            message_id = message_id_queue.popleft()
            return message_id
    return None

'''
 ack_message_queueから対象のmessage_idを含むACK結果を取り出して返します
'''
def pop_ack_message( sent_message_id ):
#    print("POP ack message : ", end="")
#    print(ack_message_queue)
    for ack in ack_message_queue: # ack[0] = STATUS, ack[1] = MSG_ID
#        print("Test : " + ack[1]+ " / Target :", end="")
#        print(sent_message_id)
        if( ack[1] in sent_message_id ):
            ack_message_queue.remove(ack)
            sent_message_id.remove(ack[1])
            return ack[0] # ack_status '0' or '1'
    return '0' # ACKが受け取れていないので'0'を返して再送

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

'''
 メインスレッド
'''
def main_thread():
    global packet_number
    e_rec = threading.Event()
    e_arq = threading.Event()
    e_slp = threading.Event()
    e_rec.clear()
    th_rec = threading.Thread(name='rec', target=receive_packet, args=(e_rec,))
    th_rec.start()
    th_route = threading.Thread(name='route', target=routing_packet, args=(e_rec,))
    th_route.start()
    print("Start threading!")
    try:
        while True:
            e_arq.clear()
            th_arq = threading.Thread(name='arq', target=automatic_repeat_request, args=(e_arq, packet_number))
            th_arq.start()
            th_arq.join(30) # 30秒で打ち切る
            e_arq.set()
            packet_number = ( packet_number+1 ) % 10
            sleep_time = broadcast_sleep_all(2)
            now  = datetime.now()
            day  = now.strftime("%Y%m%d")
            date = now.strftime("%H:%M:%S")
            fiap_upload = []
            for packet in accepted_packets:
                # packet = (send_id, packet_number, temp, humi, x_pos, y_pos)
                send_id = packet[0]
                temp = float(packet[2])
                humi = float(packet[3])
                x_pos = packet[4]
                y_pos = packet[5]
                posit = x_pos + '_' + y_pos
                svp = temp2svp(temp)  # Saturated Vapor Pressure [Pa]
                vp = svp * humi / 100 # Vapor Pressure [Pa]
                vpd = (svp-vp)/1000   # Vapour Pressure Dificit [kPa]
                csv = date+'\t'+str(temp)+'\t'+str(humi)+'\t'+str(vpd)
                with open(OUTPUT_FILE + send_id + '_' + day+'.csv', 'a') as f:
                    f.write(csv+'\r\n')
                fiap_upload.append(['http://iwata.com/IWATA/temp_'+posit, temp, now])
                fiap_upload.append(['http://iwata.com/IWATA/humi_'+posit, humi, now])
                fiap_upload.append(['http://iwata.com/IWATA/VPD_' +posit, vpd,  now])
                print(send_id+":"+posit+", ",end="")
            if( len(fiap_upload) > 0 ):
                fiap.write(fiap_upload)
            accepted_packets.clear()
#            sleep(1)
            e_slp.set()
            th_slp = threading.Thread(name='insleep', target=insleep_automatic_repeat_request, args=(e_slp, packet_number))
            th_slp.start()
            sleep_time = INTERMITTENT_TIME - datetime.now().second
            if( sleep_time > 3 ): # sleepが2秒分あって、その他1秒換算
                sleep_time -= 3
            print(" Sleep...."+str(sleep_time)+" s")
            sleep(sleep_time)
            print("[Information] Wakeup!")
            e_slp.clear()

    except KeyboardInterrupt:
        print("W: interrupt received, stopping script")
    finally:
        print("W: close...")
        s.close()
    e_rec.set()
    e_arq.set()
    e_slp.clear()
    th_rec.join()
    sys.exit()

if __name__ == '__main__':
    pid = os.fork()
    if( pid > 0 ):
        f2 = open(PID_FILE,'w')
        f2.write(str(pid)+"\n")
        f2.close()        
        sys.exit()
    if( pid == 0 ):
        main_thread()

