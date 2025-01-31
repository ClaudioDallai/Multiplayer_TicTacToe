
#ifndef CLIENT_GAME_H
#define CLIENT_GAME_H

#include "socket_guard.h"

// Clients commands (same as Server)
#define COMMAND_JOIN 0
#define COMMAND_CREATE_ROOM 1
#define COMMAND_CHALLENGE 2
#define COMMAND_MOVE 3
#define COMMAND_QUIT 4

typedef enum game_state
{
    CONNECTION, WAITING_ROOM, PLAY
} game_state;

extern int quit;
extern game_state current_client_state;


void game_init(void);
void game_deinit(void);

void on_state_switch(const game_state previous_client_state, const game_state current_client_state, const int skip_current);

void connection_process_input(void);
void connection_draw(void);

void waiting_room_process_input(void);
void waiting_room_draw(void);

void play_process_input(void);
void play_draw(void);

void load_assets(void);

void internal_reset_pressed_buttons(void);

void join_server(void);


#endif