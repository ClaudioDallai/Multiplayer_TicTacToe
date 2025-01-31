
#ifndef CLIENT_GAME_H
#define CLIENT_GAME_H

#include "socket_guard.h"



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


#endif