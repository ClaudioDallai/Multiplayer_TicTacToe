
#ifndef CLIENT_GAME_H
#define CLIENT_GAME_H

#include "socket_guard.h"

// Clients commands (same as Server)
#define COMMAND_JOIN 0
#define COMMAND_CHALLENGE 1
#define COMMAND_MOVE 2
#define COMMAND_QUIT 3
#define COMMAND_CREATE_ROOM 4

// Server possible responses
#define COMMAND_ANNOUNCE_ROOM 5 
#define SERVER_RESPONSE_OK 6
#define SERVER_RESPONSE_NEGATED 7
#define SERVER_RESPONSE_DEAD 8
#define SERVER_RESPONSE_ROOM_DESTROYED 9
#define SERVER_RESPONSE_ROOM_CREATED 10
#define SERVER_RESPONSE_KICK 11
#define SERVER_RESPONSE_ROOM_CLOSING 12
#define SERVER_GAME_PLAYFIELD_AND_TURN 13
#define SERVER_GAME_END_RESULT 14

// Client interface defines
#define MAX_IP_LENGTH 15
#define MAX_PORT_LENGTH 4
#define MAX_PLAYER_LENGTH 20
#define MAX_ROOM_ID_LENGHT 10
#define SERVER_MAX_ROOMS 10

#define BUTTON_SELECTED WHITE
#define BUTTON_UNSELECTED RED
#define GRID_SIZE 3
#define CELL_SIZE 200

// Extern vars
typedef enum game_state
{
    CONNECTION, WAITING_ROOM, PLAY
} game_state;

extern int quit;
extern game_state current_client_state;


// Methods
void game_init(void);
void game_deinit(void);

void on_state_switch(const game_state previous_client_state, const game_state current_client_state, const int compute_current);
void manage_application_exit(void);

void load_assets(void);
void internal_reset_pressed_buttons(void);
void internal_reset_client_ids(void);

void connection_process_input(void);
void connection_draw(void);

void waiting_room_process_input(void);
void waiting_room_draw(void);

void play_process_input(void);
void play_draw(void);
void draw_grid_playfield(const int screen_width_playfield, const int screen_height_playfield, const int grid[9]);

void manage_server_join(void);
void manage_server_quit(void);
void manage_server_waiting_rooms(void);
void manage_server_play_state(void);

void join_server(void);
int receive_login_response_package(void);
void challenge_room(void);
void create_server_room(void);
void join_server_room(void);
void request_for_a_move(const int cell);

#endif