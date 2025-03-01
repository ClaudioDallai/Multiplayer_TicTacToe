
#ifndef SOCKET_GUARD_H
#define SOCKET_GUARD_H

// Insert here header of socket func wrappers

int create_socket(const char* address, const int port);
int receive_packet(char *buffer, size_t buffer_size);
void send_packet(const char *buffer, size_t length);
void deinit_client();

#endif