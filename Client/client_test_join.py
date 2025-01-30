import socket
import struct
import sys

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.settimeout(3.0)

message_join_server = struct.pack('<II', 0, 0) + b'ClaudioDallaz\0\0\0\0\0\0\0'
#messagge_create_room = struct.pack('<II', 0, 4)
messagge_challenge_room = struct.pack('<III', 0, 55, int(sys.argv[1]))

s.sendto(message_join_server, ('127.0.0.1', 9999))
#s.sendto(messagge_create_room, ('127.0.0.1', 9999))
#s.sendto(messagge_challenge_room, ('127.0.0.1', 9999))

server_answer = ""

while (True):

    move_value = input("cell? >")

    if (move_value == 'q'):
        message_quit = struct.pack('<II', 0, 3)
        s.sendto(message_quit, ('127.0.0.1', 9999))
    else:
        message_move = struct.pack('<III', 0, 2, int(move_value))
        s.sendto(message_move, ('127.0.0.1', 9999))

    try:
        packet, sender = s.recvfrom(64)
    except socket.timeout:
        print("Timed out, retrying...")
        continue

    server_answer = struct.unpack('<III', packet[:12])    
    print("Packet: {}; Sender: {};".format(server_answer, sender))
   