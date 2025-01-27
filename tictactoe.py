import socket
import struct
import sys
import time

COMMAND_JOIN = 0
COMMAND_CHALLENGE = 1
COMMAND_MOVE = 2
COMMAND_QUIT = 3
COMMAND_CREATE_ROOM = 4
COMMAND_ANNOUNCE_ROOM = 5


class Room:

    def __init__(self, room_id, owner):
        self.room_id = room_id
        self.owner = owner
        self.reset()

    def is_door_open(self):
        return self.challenger is None

    def has_started(self):
        for cell in self.playfield:
            if cell is not None:
                return True
        return False

    def print_symbol(self, cell):
        player = self.playfield[cell]
        if not player:
            return " "
        if player == self.owner:
            return "X"
        if player == self.challenger:
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

    def move(self, player, cell):
        if cell < 0 or cell > 8:
            return False
        if self.playfield[cell] is not None:
            return False
        if self.winner:
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
        self.room_counter = 100
        self.address = address
        self.port = port
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.socket.settimeout(1)
        self.socket.bind((address, port))
        print("Server ready: waiting for packets...")

    def kick(self, sender):
        bad_player = self.players[sender]
        if bad_player.room:
            # if bad_player.room.has_started():
            if bad_player.room.owner == bad_player:
                self.destroy_room(bad_player.room)
            else:
                bad_player.room.reset()
        del self.players[sender]
        print("{} ({}) has been kicked".format(sender, bad_player.name))

    def destroy_room(self, room):
        del self.rooms[room.room_id]
        room.owner.room = None
        if room.challenger:
            room.challenger = None
        print("Room {} destroyed".format(room.room_id))

    def remove_player(self, sender):
        player = self.players[sender]
        if not player.room:
            del self.players[sender]
            print("Player {} removed".format(player.name))
            return
        if player == player.room.challenger:
            player.room.reset()
            del self.players[sender]
            print("Player {} removed".format(player.name))
            return
        self.destroy_room(player.room)
        del self.players[sender]
        print("Player {} removed".format(player.name))

    def tick(self):
        try:
            packet, sender = self.socket.recvfrom(64)
            print(packet, sender)
            # <II... struct.pack('<II', 0, 0) + b'roberto\0\0\0\0....'
            if len(packet) < 8:
                print("invalid packet size: {}".format(len(packet)))
                return
            rid, command = struct.unpack("<II", packet[0:8])
            if command == COMMAND_JOIN and len(packet) == 28:
                if sender in self.players:
                    print("{} has already joined!".format(sender))
                    self.kick(sender)
                    return
                self.players[sender] = Player(packet[8:28])
                print(
                    "player {} joined from {} [{} players on server]".format(
                        self.players[sender].name, sender, len(self.players)
                    )
                )
                return
            elif command == COMMAND_CREATE_ROOM:
                if sender not in self.players:
                    print("Unknown player {}".format(sender))
                    return
                player = self.players[sender]
                if player.room:
                    print(
                        "Player {} ({}) already has a room".format(sender, player.name)
                    )
                    return
                player.room = Room(self.room_counter, player)
                self.rooms[self.room_counter] = player.room
                print(
                    "Room {} for player {} ({}) created".format(
                        self.room_counter, sender, player.name
                    )
                )
                self.room_counter += 1
                player.last_packet_ts = time.time()
                return
            elif command == COMMAND_CHALLENGE and len(packet) == 12:
                if sender not in self.players:
                    print("Unknown player {}".format(sender))
                    return
                player = self.players[sender]
                if player.room:
                    print(
                        "Player {} ({}) already in a room".format(sender, player.name)
                    )
                    return
                (room_id,) = struct.unpack("<I", packet[8:12])
                if room_id not in self.rooms:
                    print("Unknown room {}".format(room_id))
                    return
                room = self.rooms[room_id]
                if not room.is_door_open():
                    print("Room {} is closed!".format(room_id))
                    return
                room.challenger = player
                player.room = room
                player.last_packet_ts = time.time()
                print("Game on room {} started!".format(room_id))
                return
            elif command == COMMAND_MOVE and len(packet) == 12:
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
                    print("player {} did WON!".format(player.room.winner.name))
                    player.room.reset()
                    return
            elif command == COMMAND_QUIT:
                if sender not in self.players:
                    print("Unknown player {}".format(sender))
                    return
                self.remove_player(sender)
                return
            else:
                print("unknown command from {}".format(sender))
        except TimeoutError:
            return
        except KeyboardInterrupt:
            sys.exit(1)
        except:
            #print(sys.exc_info())
            return

    def announces(self):
        rooms = []
        for room_id in self.rooms:
            room = self.rooms[room_id]
            if room.is_door_open():
                rooms.append(room)

        for sender in self.players:
            player = self.players[sender]
            if player.room:
                continue
            for room in rooms:
                print(
                    "announcing room {} to player {}".format(room.room_id, player.name)
                )
                packet = struct.pack("<III", 0, COMMAND_ANNOUNCE_ROOM, room.room_id)
                self.socket.sendto(packet, sender)

    def check_dead_peers(self):
        now = time.time()
        dead_players = []
        for sender in self.players:
            player = self.players[sender]
            if now - player.last_packet_ts > 30:
                dead_players.append(sender)

        for sender in dead_players:
            print('removing {} for inactivity...'.format(sender))
            self.remove_player(sender)

    def run(self):
        while True:
            self.tick()
            self.announces()
            self.check_dead_peers()


if __name__ == "__main__":
    Server("127.0.0.1", 9999).run() # Immettere indirizzo ip di chi fa da server
