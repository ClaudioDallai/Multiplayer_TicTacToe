# Multiplayer_TicTacToe
 AIV exercise of a tictactoe multiplayer game using UDP protocol:
  - Server was made in Python. 
  - Client was made in C (using Raylib).
 
---

How it works:
 - Launch server.py to initialize the Server. The address used is Local Host, port 9999.
 - Launch main.exe (for each client), insert requested data by hovering buttons and press enter.
 - Create a lobby or join an existing one (as before hover relative button to be able to write).
 - Have fun.

Notice:
- This is just a small experiment of UDP networking. 
- Server works on Local Host, port 9999. Feel free to change it at the end of server.py file. All source files are available.
- Server supports a multi-room system. More than two clients can create rooms, wait, or play.
- There's an inactivity kick server-side.
