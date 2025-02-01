#include "client_game.h"

int main (int argc, char **argv)
{
    game_init();

    game_state previos_client_state = current_client_state;
    on_state_switch(previos_client_state, current_client_state, 1);

    while (!quit)
    {
        if (previos_client_state != current_client_state)
        {
            on_state_switch(previos_client_state, current_client_state, 0);
        }

        switch (current_client_state)
        {
            case CONNECTION:
                connection_process_input();
                connection_draw();
                manage_server_join();
                break;

            case WAITING_ROOM:
                waiting_room_process_input();
                waiting_room_draw();
                break;

            case PLAY:
                break;

            default:
                break;
        }

        previos_client_state = current_client_state;
    }

    game_deinit();

    return 0;
}