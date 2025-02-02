// Include
#include "client_game.h"
#include "raylib.h"

#include "stdlib.h"
#include "stdio.h"
#include "stdint.h"
#include "string.h"

// Const vars
const int screen_width = 800;
const int screen_height = 600;
const int target_fps = 60;

const int screen_width_playfield = GRID_SIZE*CELL_SIZE;
const int screen_height_playfield = GRID_SIZE*CELL_SIZE;

// Resources
const char* font_path = "./resources/setback.png";

// Client application related vars
int quit = 0;
game_state current_client_state = CONNECTION;
int turn_token = 0;

int grid[GRID_SIZE * GRID_SIZE] = {0};
float master_volume_value = 0.5f;
Font font;

char ip_address[MAX_IP_LENGTH + 1] = {0};
int ip_index = 0;
int address_text_pressed = 0;
Rectangle address_button = {160.0f, 210.0f, 160.0f, 30.0f};

char port[MAX_PORT_LENGTH + 1] = {0};
int port_index = 0;
int port_text_pressed = 0;
Rectangle port_button = {160.0f, 280.0f, 160.0f, 30.0f};

char player_name[MAX_PLAYER_LENGTH + 1] = {0};
int player_name_index = 0;
int player_text_pressed = 0;
Rectangle player_name_button = {160.0f, 350.0f, 160.0f, 30.0f};

int create_room_text_pressed = 0;
int already_in_a_room = 0;
int client_room_ids[SERVER_MAX_ROOMS] = {0};
int current_client_created_room_id = -1;
char room_created_text[64] = {0};
Rectangle create_room_button = {30.0f, 30.0f, 310.0f, 30.0f};

char target_room_id[MAX_ROOM_ID_LENGHT + 1] = {0};
int target_room_id_index = 0;
int target_room_id_text_pressed = 0;
Rectangle target_room_id_button = {400.0f, 30.0f, 250.0f, 30.0f};



// Init
void game_init(void)
{
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(screen_width, screen_height, "Client TicTacToe");

    InitAudioDevice();
    SetMasterVolume(master_volume_value);
    
    SetTargetFPS(target_fps);

    load_assets();
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

void on_state_switch(const game_state previous_client_state, const game_state current_client_state, const int compute_current)
{
    switch (previous_client_state)
    {
        case CONNECTION:
            internal_reset_pressed_buttons();
            // Maybe reset old chars* ...
            break;
        case WAITING_ROOM:
            create_room_text_pressed = 0;
            internal_reset_client_ids();
            break;
        case PLAY:
            break;
        default:
            break;
    }

    if (compute_current)
    {
        switch (current_client_state)
        {
            case CONNECTION:
                deinit_client();
                break;
            case WAITING_ROOM:
                already_in_a_room = 0;
                turn_token = 0;
                internal_reset_client_ids();
                break;
            case PLAY:
                for (int i = 0; i <= (GRID_SIZE * GRID_SIZE); i++)
                {
                    grid[i] = 0;
                }
                break;
            default:
                break;
        }
    }
}

int receive_response_package(void)
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

void internal_reset_client_ids(void)
{
    for (int i = 0; i < SERVER_MAX_ROOMS; i++)
    {
        client_room_ids[i] = 0;
    }

    target_room_id_index = 0;
    target_room_id_text_pressed = 0;

    for (int i = 0; i <= MAX_ROOM_ID_LENGHT; i++)
    {
        target_room_id[i] = '\0';
    }
}


void manage_application_exit(void)
{
    if (WindowShouldClose())
    {
        if (current_client_state != CONNECTION)
        {
            manage_server_quit();
        }

        quit = 1;
    }
}


void manage_server_quit(void)
{
    char buffer[8] = {0};
    uint32_t quit_command = COMMAND_QUIT;
    memcpy(buffer, &quit_command, sizeof(quit_command));
    send_packet(buffer, sizeof(buffer));
}

void manage_server_join(void)
{
    int server_response = receive_response_package();
    if (server_response == SERVER_RESPONSE_OK)
    {
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

void create_server_room(void)
{
    printf("Ask for room creation\n");
    char buffer[8] = {0}; 
    uint32_t create_room_command = COMMAND_CREATE_ROOM;
    memcpy(buffer, &create_room_command, sizeof(create_room_command));
    memcpy(buffer + sizeof(create_room_command), &create_room_command, sizeof(create_room_command)); 
    send_packet(buffer, sizeof(buffer));
}

void challenge_room(void)
{
    int room_to_challenge = atoi(target_room_id);
    int room_found = 0;

    for (int i = 0; i < SERVER_MAX_ROOMS; i++)
    {
        if (client_room_ids[i] == room_to_challenge)
        {
            room_found = 1;
            break;
        }
    }

    if (room_found)
    {
        printf("Ask for challenge room: %d\n", room_found);
        char buffer[8] = {0}; 
        uint32_t challenge_room_command = COMMAND_CHALLENGE;
        memcpy(buffer, &challenge_room_command, sizeof(challenge_room_command));
        memcpy(buffer + sizeof(challenge_room_command), &room_to_challenge, sizeof(room_to_challenge)); 
        send_packet(buffer, sizeof(buffer));
    }
}

void manage_server_waiting_rooms(void)
{
    char buffer[64]= {0};
    int bytes_received = receive_packet(buffer, sizeof(buffer));
    uint32_t command;

    if (bytes_received >= 4)
    {
        memcpy(&command, buffer, sizeof(command));
    }

    if (bytes_received == 4)
    {
        printf("Command: %u\n", command);

        switch (command)
        {
            case SERVER_RESPONSE_OK:
                current_client_state = PLAY;
                break;
            case SERVER_RESPONSE_KICK:
                current_client_state = CONNECTION;
            default:
                break;
        }
    }

    if(bytes_received == 8) 
    {
        printf("Command: %u\n", command);

        uint32_t room_id;
        memcpy(&room_id, buffer + sizeof(command), sizeof(room_id));

        switch (command)
        {
            case SERVER_RESPONSE_ROOM_DESTROYED:
                already_in_a_room = 0;
                current_client_created_room_id = -1;
                printf("Room %u was destroyed!\n", room_id);
                break;
            case SERVER_RESPONSE_ROOM_CREATED:
                already_in_a_room = 1;
                current_client_created_room_id = room_id;
                sprintf(room_created_text, "Room %d was created!", room_id);
                printf("Room %u was created!\n", current_client_created_room_id);
                internal_reset_client_ids();
                break;
            default:
                break;
        }
    }

    if (bytes_received == ((4 * SERVER_MAX_ROOMS) + 4))
    {
        printf("Command: %u\n", command);

        if (command == COMMAND_ANNOUNCE_ROOM)
        {
           memcpy(client_room_ids, buffer + sizeof(command), sizeof(client_room_ids));
        }
    }
}


void waiting_room_process_input(void)
{
    manage_application_exit();

   if (CheckCollisionPointRec(GetMousePosition(), create_room_button) && !already_in_a_room)
    {
        create_room_text_pressed = 1;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsKeyPressed(KEY_ENTER))
        {
            create_room_text_pressed = 0;
            create_server_room();
        }
    }
    else if(CheckCollisionPointRec(GetMousePosition(), target_room_id_button) && !already_in_a_room)
    {
        target_room_id_text_pressed = 1;
        int key= GetCharPressed();
        
        if (target_room_id_index < MAX_ROOM_ID_LENGHT && (key >= 46 && key <= 57))
        {
            target_room_id[target_room_id_index] = (char)key;
            target_room_id_index++;
            target_room_id[target_room_id_index] = '\0';
        }
        if (IsKeyPressed(KEY_BACKSPACE) && target_room_id_index > 0) 
        {
            target_room_id_index--;
            target_room_id[target_room_id_index] = '\0';
        }
    }
    else
    {
        create_room_text_pressed = 0;
        target_room_id_text_pressed = 0;
    }

    if (IsKeyPressed(KEY_ENTER) && port_index > 0 && !already_in_a_room) 
    {
        challenge_room();
    }
}

void waiting_room_draw(void)
{
    BeginDrawing();
    ClearBackground(BLACK);

    if (create_room_text_pressed)
    {
        DrawTextEx(font, "| Create New Room |", (Vector2){30.0f, 30.0f}, 30.0f, 1.0f, BUTTON_SELECTED);
    }
    else
    {
        DrawTextEx(font, "| Create New Room |", (Vector2){30.0f, 30.0f}, 30.0f, 1.0f, BUTTON_UNSELECTED);
    }

    if (target_room_id_text_pressed)
    {
        DrawTextEx(font, "Select Room ID :", (Vector2){400.0f, 30.0f}, 30.0f, 1.0f, BUTTON_SELECTED);
        DrawTextEx(font, target_room_id, (Vector2){400.0f, 75.0f}, 30.0f, 5.0f, BUTTON_SELECTED);
    }
    else
    {
        DrawTextEx(font, "Select Room ID :", (Vector2){400.0f, 30.0f}, 30.0f, 1.0f, BUTTON_UNSELECTED);
        DrawTextEx(font, target_room_id, (Vector2){400.0f, 75.0f}, 30.0f, 5.0f, BUTTON_UNSELECTED);
    }

    if (already_in_a_room)
    {
        DrawTextEx(font, room_created_text, (Vector2){30.0f, 75.0f}, 30.0f, 1.0f, WHITE);
    }

    float yOffset = 50;
    char text_available_rooms[50] = {0};
    for (int i = 0; i < SERVER_MAX_ROOMS; i++)
    {
        if (client_room_ids[i] == 0)
        {
            continue;
        }

        sprintf(text_available_rooms, "Room ID : %d", client_room_ids[i]);
        DrawTextEx(font, text_available_rooms, (Vector2){30.0f, 120.0f + (yOffset * i)}, 30.0f, 1.0f, RED);
    }

    EndDrawing();
}

void play_process_input(void)
{
    manage_application_exit();

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mouse_pos = GetMousePosition();

        int cell_width = screen_width_playfield / GRID_SIZE; 
        int cell_height = screen_height_playfield / GRID_SIZE;

        int col = (mouse_pos.x - 100) / cell_width;
        int row = mouse_pos.y / cell_height;
        int index = row * GRID_SIZE + col;
       
        request_for_a_move(index);
    }
}

void draw_grid_playfield(const int screen_width_playfield, const int screen_height_playfield, const int grid[9])
{
    int cell_width = screen_width_playfield / GRID_SIZE;
    int cell_height = screen_height_playfield / GRID_SIZE;

    for (int row = 0; row < GRID_SIZE; row++) {
        for (int col = 0; col < GRID_SIZE; col++) {
            int x = col * cell_width + 100;
            int y = row * cell_height;
            int index = row * GRID_SIZE + col;
            Rectangle button = { x, y, cell_width, cell_height };
            DrawRectangleRec(button, BLACK);
            DrawRectangleLinesEx(button, 2, RED);
            if (grid[index] == 1) // An X is present on current cell
            {
                DrawLine(x + 20, y + 20, x + cell_width - 20, y + cell_height - 20, RED);
                DrawLine(x + 20, y + cell_height - 20, x + cell_width - 20, y + 20, RED);
            } 
            else if (grid[index] == 2) // A O is present on current cell
            {
                DrawCircleLines(x + cell_width / 2, y + cell_height / 2, cell_width / 3, RED);
            }
        }
    }
}

void play_draw(void)
{
    BeginDrawing();
    ClearBackground(BLACK);

    draw_grid_playfield(screen_width_playfield, screen_height_playfield, grid);
    if (turn_token)
    {
        DrawTextEx(font, "Your", (Vector2){10.0f, 40.0f}, 30.0f, 1.0f, RED);
        DrawTextEx(font, "Turn!", (Vector2){10.0f, 80.0f}, 30.0f, 1.0f, RED);
    }
    else
    {
        DrawTextEx(font, "Not", (Vector2){10.0f, 40.0f}, 30.0f, 1.0f, RED);
        DrawTextEx(font, "Your", (Vector2){10.0f, 80.0f}, 30.0f, 1.0f, RED);
        DrawTextEx(font, "Turn!", (Vector2){10.0f, 120.0f}, 30.0f, 1.0f, RED);
    }

    EndDrawing();
}

void manage_server_play_state(void)
{
    char buffer[64]= {0};
    int bytes_received = receive_packet(buffer, sizeof(buffer));
    uint32_t command;

    if (bytes_received >= 4)
    {
        memcpy(&command, buffer, sizeof(command));
    }

    if (bytes_received == 4)
    {
        printf("Command: %u\n", command);

        switch (command)
        {
            case SERVER_RESPONSE_OK:
                break;
            case SERVER_RESPONSE_KICK:
                current_client_state = CONNECTION;
                break;
            case SERVER_RESPONSE_ROOM_CLOSING:
                current_client_state = WAITING_ROOM;
                break;
            default:
                break;
        }

        return;
    }

    if (bytes_received == 44)
    {
        printf("Command: %u\n", command);
        if (command == SERVER_GAME_PLAYFIELD_AND_TURN)
        {
            memcpy(&turn_token, buffer + sizeof(command), sizeof(turn_token));
            memcpy(grid, buffer + sizeof(command) + sizeof(turn_token), sizeof(grid));
            printf("received playfield and turn %d\n", turn_token);
        }

        return;
    }
}

void request_for_a_move(const int cell)
{
    if (turn_token)
    {
        char buffer[8] = {0}; 
        uint32_t move_command = COMMAND_MOVE;
        memcpy(buffer, &move_command, sizeof(move_command));
        memcpy(buffer + sizeof(move_command), &cell, sizeof(cell)); 
        send_packet(buffer, sizeof(buffer));
    }
    else
    {
        printf("Not your Turn!\n");
    }
}