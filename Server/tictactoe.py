import socket
import struct
import sys
import time

LOWER_COMMAND_VALUE = 0
UPPER_COMMAND_VALUE = 5

COMMAND_JOIN = 0
COMMAND_CHALLENGE = 1
COMMAND_MOVE = 2
COMMAND_QUIT = 3
COMMAND_CREATE_ROOM = 4
COMMAND_ANNOUNCE_ROOM = 5

SERVER_RESPONSE_OK = 6
SERVER_RESPONSE_NEGATED = 7
SERVER_RESPONSE_DEAD = 8
SERVER_RESPONSE_ROOM_DESTROYED = 9
SERVER_RESPONSE_ROOM_CREATED = 10
SERVER_RESPONSE_KICK = 11
SERVER_RESPONSE_ROOM_CLOSING = 12

SERVER_GAME_TURN_RULE = 13

KICK_TIME = 40
PASSIVE_ROOM_ANNOUNCEMENT_TIME = 5
MAX_ROOMS = 10


class Room:

    def __init__(self, room_id, owner):
        self.room_id = room_id
        self.owner = owner
        self.reset()

    def is_door_open(self):
        return self.challenger is None and self.winner is None and self.draw is False

    def has_started(self):
        for cell in self.playfield:
            if cell is not None:
                return True
        return False

    def print_symbol(self, cell):
        current_player = self.playfield[cell]
        if not current_player:
            return " "
        if current_player == self.owner:
            return "X"
        if current_player == self.challenger:
            return "O"
        else:
            return "?"

    def print_playfield(self):
        print(
            "|{}|{}|{}|".format(
                self.print_symbol(0), self.print_symbol(1), self.print_symbol(2)
            )
        )
        print(
            "|{}|{}|{}|".format(
                self.print_symbol(3), self.print_symbol(4), self.print_symbol(5)
            )
        )
        print(
            "|{}|{}|{}|".format(
                self.print_symbol(6), self.print_symbol(7), self.print_symbol(8)
            )
        )

    def reset(self):
        self.challenger = None
        self.playfield = [None] * 9
        self.turn = self.owner
        self.winner = None
        self.draw = False

    def check_horizontal(self, row):
        for col in range(0, 3):
            if self.playfield[row * 3 + col] is None:
                return None
        player = self.playfield[row * 3]
        if self.playfield[row * 3 + 1] != player:
            return None
        if self.playfield[row * 3 + 2] != player:
            return None
        return player

    def check_vertical(self, col):
        for row in range(0, 3):
            if self.playfield[row * 3 + col] is None:
                return None
        player = self.playfield[col]
        if self.playfield[3 + col] != player:
            return None
        if self.playfield[6 + col] != player:
            return None
        return player

    def check_diagonal_left(self):
        for cell in (0, 4, 8):
            if self.playfield[cell] is None:
                return None
        player = self.playfield[0]
        if self.playfield[4] != player:
            return None
        if self.playfield[8] != player:
            return None
        return player

    def check_diagonal_right(self):
        for cell in (2, 4, 6):
            if self.playfield[cell] is None:
                return None
        player = self.playfield[2]
        if self.playfield[4] != player:
            return None
        if self.playfield[6] != player:
            return None
        return player

    def check_victory(self):
        for row in range(0, 3):
            winner = self.check_horizontal(row)
            if winner:
                return winner
        for col in range(0, 3):
            winner = self.check_vertical(col)
            if winner:
                return winner
        winner = self.check_diagonal_left()
        if winner:
            return winner
        return self.check_diagonal_right()
    
    def check_draw(self):
        for cell in self.playfield:
            if cell is None:
                return False
        return True

    def move(self, player, cell):
        if cell < 0 or cell > 8:
            return False
        if self.playfield[cell] is not None:
            return False
        if self.winner:
            return False
        if self.draw:
            return False
        if self.challenger is None:
            return False
        if player.room != self:
            return False
        if player != self.owner and player != self.challenger:
            return False
        if player != self.turn:
            return False
        self.playfield[cell] = player
        self.winner = self.check_victory()
        self.draw = self.check_draw()
        self.turn = self.challenger if self.turn == self.owner else self.owner
        return True


class Player:

    def __init__(self, name):
        self.name = name
        self.room = None
        self.last_packet_ts = time.time()


class Server:

    def __init__(self, address, port):
        self.players = {}
        self.rooms = {}
        self.room_counter = 1
        self.last_announcement = time.time()
        self.address = address
        self.port = port
        self.server_clientactions_resolution = {
            COMMAND_JOIN: self.command_join_resolution,
            COMMAND_CHALLENGE: self.command_challenge_resolution,
            COMMAND_MOVE: self.command_move_resolution,
            COMMAND_QUIT: self.command_quit_resolution,
            COMMAND_CREATE_ROOM: self.command_create_room_resolution,
        }

        self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.socket.settimeout(1)
        self.socket.bind((address, port))
        print("Server ready: waiting for packets...")

    def kick(self, sender):
        bad_player = self.players[sender]
        if bad_player.room:
            if bad_player.room.owner == bad_player:
                self.destroy_room(sender, bad_player.room)
            else:
                bad_player.room.reset()
        del self.players[sender]
        print("{} ({}) has been kicked".format(sender, bad_player.name))
        self.server_response(sender, SERVER_RESPONSE_KICK)
        self.announces()

    def destroy_room(self, player, room):
        if room is None:
            packet = struct.pack("<II", SERVER_RESPONSE_ROOM_DESTROYED, room.room_id)
            self.socket.sendto(packet, player)
            self.announces()
            return
        if room.room_id in self.rooms:
            packet = struct.pack("<II", SERVER_RESPONSE_ROOM_CLOSING, room.room_id)
            self.socket.sendto(packet, self.rooms[room.room_id][1])
            del self.rooms[room.room_id]
            self.announces()
            
        room.owner.room = None
        if room.challenger:
            room.challenger = None
        packet = struct.pack("<II", SERVER_RESPONSE_ROOM_DESTROYED, room.room_id)
        self.socket.sendto(packet, player)
        print("Room {} destroyed".format(room.room_id))
        self.announces()

    def remove_player(self, sender):
        print("PLAYER REMOVAL REQUESTED!")
        player = self.players[sender]
        if not player.room:
            print("Player not playing {} removed".format(player.name))
            del self.players[sender]
            self.server_response(sender, SERVER_RESPONSE_KICK) # just notify leaving player
            return
        if player == player.room.challenger:
            print("Player challenger {} removed".format(player.name))
            player.room.reset()
            del self.players[sender]
            self.server_response(self.rooms[player.room.room_id][1], SERVER_RESPONSE_ROOM_CLOSING) # Notify owner or the room that challenger left
            self.server_response(sender, SERVER_RESPONSE_KICK)
            return
        self.destroy_room(sender, player.room)
        print("Player owner {} removed".format(player.name))
        del self.players[sender]
        self.server_response(sender, SERVER_RESPONSE_KICK) # Room owner was kicked first

    def server_response(self, client, response):
        packet = struct.pack("<I", response)
        self.socket.sendto(packet, client)
        

# From Client - commands resolution
    def command_join_resolution(self, packet, sender):
        if len(packet) == 25:
            if sender in self.players:
                print("{} has already joined!".format(sender))
                self.kick(sender)
                self.server_response(sender, SERVER_RESPONSE_NEGATED)
                return
            self.players[sender] = Player(packet[4:25])
            self.server_response(sender, SERVER_RESPONSE_OK)
            print("player {} joined from {} [{} players on server]".format(self.players[sender].name, sender, len(self.players)))
            self.announces()
            return
        else:
            self.server_response(sender, SERVER_RESPONSE_NEGATED)
            print("Join request packet size invalid: {}".format(len(packet)))

            
        
    def command_create_room_resolution(self, packet, sender):
        if len(packet) == 8:
            if sender not in self.players:
                print("Unknown player {}".format(sender))
                self.server_response(sender, SERVER_RESPONSE_NEGATED)
                return
            player = self.players[sender]
            if player.room:
                print("Player {} ({}) already has a room".format(sender, player.name))
                self.server_response(sender, SERVER_RESPONSE_NEGATED)
                return
            if (len(self.rooms)>= MAX_ROOMS):
                print("Max limit of rooms")
                self.server_response(sender, SERVER_RESPONSE_NEGATED)
                return
            
            player.room = Room(self.room_counter, player)
            self.rooms[self.room_counter] = (player.room, sender) # Save room and IP Owner
            print("Room {} for player {} ({}) created".format(self.room_counter, sender, player.name))
            packet = struct.pack("<II", SERVER_RESPONSE_ROOM_CREATED, self.room_counter)
            self.socket.sendto(packet, sender)
            self.room_counter += 1
            player.last_packet_ts = time.time()
            self.announces()
            return
        
    def command_challenge_resolution(self, packet, sender):
        if len(packet) == 8:
            if sender not in self.players:
                print("Unknown player {}".format(sender))
                self.server_response(sender, SERVER_RESPONSE_NEGATED)
                return
            player = self.players[sender]
            if player.room:
                print("Player {} ({}) already in a room".format(sender, player.name))
                self.server_response(sender, SERVER_RESPONSE_NEGATED)
                return
            (room_id,) = struct.unpack("<I", packet[4:8])
            if room_id not in self.rooms:
                print("Unknown room {}".format(room_id))
                self.server_response(sender, SERVER_RESPONSE_NEGATED)
                return
            room = self.rooms[room_id][0]
            if not room.is_door_open():
                print("Room {} is closed!".format(room_id))
                self.server_response(sender, SERVER_RESPONSE_NEGATED)
                return
            
            room.challenger = player
            player.room = room
            player.last_packet_ts = time.time()
            print("Game on room {} started!".format(room_id))
            self.server_response(sender, SERVER_RESPONSE_OK)
            self.server_response(self.rooms[room_id][1], SERVER_RESPONSE_OK) # Notify room owner
            self.announces()
            return

    def command_move_resolution(self, packet, sender):
        if len(packet) == 12:
            if sender not in self.players:
                print("Unknown player {}".format(sender))
                return
            player = self.players[sender]
            if not player.room:
                print("Player {} ({}) is not in a room".format(sender, player.name))
                return
            (cell,) = struct.unpack("<I", packet[8:12])
            if not player.room.move(player, cell):
                print("player {} did an invalid move!".format(player.name))
                return
            
            player.last_packet_ts = time.time()
            player.room.print_playfield()
            if player.room.winner:
                print("player {} from room {} did WON!".format(player.room.winner.name, player.room.room_id))
                player.room.reset()
                return
            if player.room.draw:
                print("Room {} ended in draw!".format(player.room.room_id))
                player.room.reset()
                return
    
    def command_quit_resolution(self, packet, sender):
        if len(packet) == 8:
            if sender not in self.players:
                print("Unknown player {}".format(sender))
                return
            self.remove_player(sender)
            self.announces()
            return
        

    def tick(self):
        try:
            packet, sender = self.socket.recvfrom(64)
            print(packet, sender)
            # <II... struct.pack('<II', 0, 0) + b'roberto\0\0\0\0....'
            if len(packet) < 8:
                print("invalid packet size: {}".format(len(packet)))
                return
            (command,) = struct.unpack("<I", packet[0:4]) 

            if command < LOWER_COMMAND_VALUE or command > UPPER_COMMAND_VALUE:
                raise Exception("Invalid {} command received".format(command))

            self.server_clientactions_resolution[command](packet, sender)
           
        except TimeoutError:
            return
        except socket.timeout:
            return
        except KeyboardInterrupt:
            sys.exit(1)
        except:
            print(sys.exc_info())
            return

    def announces(self):
        rooms = []
        room_ids = [0] * MAX_ROOMS
        counter = 0
        format = f"<{MAX_ROOMS + 1}I"

        for room_id in self.rooms:
            room = self.rooms[room_id][0]
            if room.is_door_open():
                rooms.append(room)
                room_ids[counter] = room_id
                counter += 1

        for sender in self.players:
            player = self.players[sender]
            if player.room:
                continue

            packet = struct.pack(format, COMMAND_ANNOUNCE_ROOM, *room_ids)
            self.socket.sendto(packet, sender)

    def check_dead_peers(self):
        now = time.time()
        dead_players = []
        for sender in self.players:
            player = self.players[sender]
            if now - player.last_packet_ts > KICK_TIME:
                dead_players.append(sender)

        for sender in dead_players:
            print('removing {} for inactivity...'.format(sender))
            self.remove_player(sender)
        else:
            if len(dead_players) > 0:
                self.announces()
        

    def run(self):
        while True:
            self.tick()

            now = time.time()
            if ((now - self.last_announcement) > PASSIVE_ROOM_ANNOUNCEMENT_TIME):
                self.last_announcement = now
                self.announces()

            self.check_dead_peers()


if __name__ == "__main__":
    Server("127.0.0.1", 9999).run() # Immettere indirizzo ip di chi fa da server
