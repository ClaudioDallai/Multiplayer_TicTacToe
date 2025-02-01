
#ifndef CLIENT_GAME_H
#define CLIENT_GAME_H

#include "socket_guard.h"

// Clients commands (same as Server)
#define COMMAND_JOIN 0
#define COMMAND_CHALLENGE 1
#define COMMAND_MOVE 2
#define COMMAND_QUIT 3
#define COMMAND_CREATE_ROOM 4

// Server only commands
#define COMMAND_ANNOUNCE_ROOM 5 

// Server possible responses
#define SERVER_RESPONSE_OK 6
#define SERVER_RESPONSE_NEGATED 7
#define SERVER_RESPONSE_DEAD 8
#define SERVER_RESPONSE_ROOM_DESTROYED 9
#define SERVER_RESPONSE_ROOM_CREATED 10
#define SERVER_RESPONSE_KICK 11

#define MAX_IP_LENGTH 15
#define MAX_PORT_LENGTH 4
#define MAX_PLAYER_LENGTH 20
#define MAX_ROOM_ID_LENGHT 10
#define SERVER_MAX_ROOMS 10

#define BUTTON_SELECTED WHITE
#define BUTTON_UNSELECTED RED

typedef enum game_state
{
    CONNECTION, WAITING_ROOM, PLAY
} game_state;

extern int quit;
extern game_state current_client_state;



void game_init(void);
void game_deinit(void);

void on_state_switch(const game_state previous_client_state, const game_state current_client_state, const int compute_current);
void manage_application_exit(void);

void connection_process_input(void);
void connection_draw(void);

void waiting_room_process_input(void);
void waiting_room_draw(void);

void play_process_input(void);
void play_draw(void);

void load_assets(void);
void internal_reset_pressed_buttons(void);
void internal_reset_client_ids(void);

void join_server(void);
int receive_response_package(void);

void manage_server_join(void);
void manage_server_quit(void);

void challenge_room(void);

void manage_server_waiting_rooms(void);
void create_server_room(void);
void join_server_room(void);

#endif