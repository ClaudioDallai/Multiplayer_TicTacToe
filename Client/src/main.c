#include "client_game.h"

int main (int argc, char **argv)
{
    game_init();

    game_state previos_client_state = current_client_state;
    on_state_switch(previos_client_state, current_client_state, 0);

    while (!quit)
    {
        if (previos_client_state != current_client_state)
        {
            on_state_switch(previos_client_state, current_client_state, 1);
            previos_client_state = current_client_state;
        }

        switch (current_client_state)
        {
            case CONNECTION:
                connection_process_input();
                connection_draw();
                manage_server_join();
                break;

            case WAITING_ROOM:
                manage_server_waiting_rooms();
                waiting_room_process_input();
                waiting_room_draw();
                break;

            case PLAY:
                manage_server_play_state();
                play_process_input();
                play_draw();
                break;

            default:
                break;
        }
    }

    game_deinit();

    return 0;
}