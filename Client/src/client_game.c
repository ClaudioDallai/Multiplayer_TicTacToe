// Include
#include "client_game.h"
#include "raylib.h"

#include "stdlib.h"
#include "stdio.h"
#include "stdint.h"
#include "string.h"

// Vars
const int screen_width = 800;
const int screen_height = 600;
const int target_fps = 60;

const char* font_path = "./resources/setback.png";

float master_volume_value = 0.5f;

int quit = 0;
game_state current_client_state = CONNECTION;

Font font;

#define MAX_IP_LENGTH 15
char ip_address[MAX_IP_LENGTH + 1] = {0};
int ip_index = 0;
int address_text_pressed = 0;
Rectangle address_button = {160.0f, 210.0f, 160.0f, 30.0f};

#define MAX_PORT_LENGTH 4
char port[MAX_PORT_LENGTH + 1] = {0};
int port_index = 0;
int port_text_pressed = 0;
Rectangle port_button = {160.0f, 280.0f, 160.0f, 30.0f};

#define MAX_PLAYER_LENGTH 20
char player_name[MAX_PLAYER_LENGTH + 1] = {0};
int player_name_index = 0;
int player_text_pressed = 0;
Rectangle player_name_button = {160.0f, 350.0f, 160.0f, 30.0f};

#define BUTTON_SELECTED WHITE
#define BUTTON_UNSELECTED RED



// Init
void game_init(void)
{
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(screen_width, screen_height, "Client TicTacToe");

    InitAudioDevice();
    SetMasterVolume(master_volume_value);
    
    SetTargetFPS(target_fps);

    load_assets();

    internal_reset_pressed_buttons();
}

void load_assets(void)
{
    font = LoadFont(font_path);
}

void game_deinit(void)
{
    UnloadFont(font);
    CloseAudioDevice();
    deinit_client();
}



// Game Loop

void on_state_switch(const game_state previous_client_state, const game_state current_client_state, const int skip_current)
{
    switch (previous_client_state)
    {
        case CONNECTION:
            break;
        case WAITING_ROOM:
            break;
        case PLAY:
            break;
        default:
            break;
    }

    if (!skip_current)
    {
        switch (current_client_state)
        {
            case CONNECTION:
                break;
            case WAITING_ROOM:
                break;
            case PLAY:
                break;
            default:
                break;
        }
    }
}

int receive_package(void)
{
    char buffer[64]= {0};
    int bytes_received = receive_packet(buffer, sizeof(buffer));
    if(bytes_received >= 4) // At least a response was detected (integer)
    {
        uint32_t command;
        memcpy(&command, buffer, sizeof(command));

        switch (command)
        {
            case SERVER_RESPONSE_OK:
                if (bytes_received == 4)
                {
                    printf("Server response was OK: %u\n", command);
                    return SERVER_RESPONSE_OK;
                }
                break;
            case SERVER_RESPONSE_NEGATED:
                if (bytes_received == 4)
                {
                    printf("Server response was NEGATED: %u\n", command);
                    return SERVER_RESPONSE_NEGATED;
                }
                break;
            case SERVER_RESPONSE_DEAD:
                if(bytes_received == 4)
                {
                    printf("Server is DEAD: %u\n", command);
                    return SERVER_RESPONSE_DEAD;
                }
                break;
            default:
                printf("Unknown command received: %u\n", command);
                return -1;
        }
    }
    
    return -1;
}


void internal_reset_pressed_buttons(void)
{
    address_text_pressed = 0;
    port_text_pressed = 0;
    player_text_pressed = 0;
}

void manage_application_exit(void)
{
    if (WindowShouldClose())
    {
        if (current_client_state != CONNECTION)
        {
            // TODO: Avvisa il server prima di chiudere il socket
        }
        
        quit = 1;
    }
}

void manage_server_join(void)
{
    int server_response = receive_package();
    if (server_response == SERVER_RESPONSE_OK)
    {
        internal_reset_pressed_buttons();
        current_client_state = WAITING_ROOM;
    }
}


void connection_process_input(void)
{
    manage_application_exit();
    
    if (CheckCollisionPointRec(GetMousePosition(), address_button)) 
    {
        address_text_pressed = 1;
        port_text_pressed = 0;
        player_text_pressed = 0;
    }
    else if (CheckCollisionPointRec(GetMousePosition(), port_button))
    {
        address_text_pressed = 0;
        port_text_pressed = 1;
        player_text_pressed = 0;
    }
    else if (CheckCollisionPointRec(GetMousePosition(), player_name_button))
    {
        address_text_pressed = 0;
        port_text_pressed = 0;
        player_text_pressed = 1;
    }
    else
    {
        internal_reset_pressed_buttons();
    }

    int key= GetCharPressed();
    if (address_text_pressed)
    {
        if (ip_index < MAX_IP_LENGTH && (key >= 46 && key <= 57))
        {
            ip_address[ip_index] = (char)key;
            ip_index++;
            ip_address[ip_index] = '\0';
        }
        if (IsKeyPressed(KEY_BACKSPACE) && ip_index > 0) 
        {
            ip_index--;
            ip_address[ip_index] = '\0';
        }
    }

    if (port_text_pressed)
    {
        if (port_index < MAX_PORT_LENGTH && (key >= 48 && key <= 57))
        {
            port[port_index] = (char)key;
            port_index++;
            port[port_index] = '\0';
        }
        if (IsKeyPressed(KEY_BACKSPACE) && port_index > 0) 
        {
            port_index--;
            port[port_index] = '\0';
        }
    }

    if (player_text_pressed)
    {
        if (player_name_index < MAX_PLAYER_LENGTH && key != 0)
        {
            player_name[player_name_index] = (char)key;
            player_name_index++;
            player_name[player_name_index] = '\0';
        }
        if (IsKeyPressed(KEY_BACKSPACE) && player_name_index > 0) 
        {
            player_name_index--;
            player_name[player_name_index] = '\0';
        }
    }

    if (IsKeyPressed(KEY_ENTER) && ip_index > 8 && port_index > 0 && player_name_index > 0) 
    {
        // join server
        if (create_socket(ip_address, atoi(port)))
        {
            printf("Socket creation failed\n");
        }

        join_server();
    }
}

void join_server(void)
{
    char buffer[25] = {0}; 
    uint32_t join_command = COMMAND_JOIN;
    memcpy(buffer, &join_command, sizeof(join_command));
    memcpy(buffer + sizeof(join_command), player_name, sizeof(player_name)); 
    send_packet(buffer, sizeof(buffer));
}

void connection_draw(void)
{
    BeginDrawing();
    ClearBackground(BLACK);

    DrawTextEx(font, "Insert IP, PORT, NAME to connect.", (Vector2){100, 80}, 30.0f, 1.0f, RED);
    DrawTextEx(font, "Hover them to write!", (Vector2){100, 130}, 30.0f, 1.0f, RED);


    if (address_text_pressed)
    {
        DrawTextEx(font, "Address: ", (Vector2){160, 210}, 30.0f, 1.0f, BUTTON_SELECTED);
        DrawTextEx(font, ip_address, (Vector2){320, 210}, 30.0f, 5.0f, BUTTON_SELECTED);
    }
    else
    {
        DrawTextEx(font, "Address: ", (Vector2){160, 210}, 30.0f, 1.0f, BUTTON_UNSELECTED);
        DrawTextEx(font, ip_address, (Vector2){320, 210}, 30.0f, 5.0f, BUTTON_UNSELECTED);
    }

    if (port_text_pressed)
    {
        DrawTextEx(font, "Port: ", (Vector2){160, 280}, 30.0f, 1.0f, BUTTON_SELECTED);
        DrawTextEx(font, port, (Vector2){260, 280}, 30.0f, 5.0f, BUTTON_SELECTED);
    }
    else
    {
        DrawTextEx(font, "Port: ", (Vector2){160, 280}, 30.0f, 1.0f, BUTTON_UNSELECTED);
        DrawTextEx(font, port, (Vector2){260, 280}, 30.0f, 5.0f, BUTTON_UNSELECTED);
    }

    if (player_text_pressed)
    {
        DrawTextEx(font, "Player Name: ", (Vector2){160, 350}, 30.0f, 1.0f, BUTTON_SELECTED);
        DrawTextEx(font, player_name, (Vector2){380, 350}, 30.0f, 5.0f, BUTTON_SELECTED);
    }
    else
    {
        DrawTextEx(font, "Player Name: ", (Vector2){160, 350}, 30.0f, 1.0f, BUTTON_UNSELECTED);
        DrawTextEx(font, player_name, (Vector2){380, 350}, 30.0f, 5.0f, BUTTON_UNSELECTED);
    }


    EndDrawing();
}

void waiting_room_process_input(void)
{
    manage_application_exit();
}

void waiting_room_draw(void)
{
    BeginDrawing();
    ClearBackground(BLACK);

    EndDrawing();
}