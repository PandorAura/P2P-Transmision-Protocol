// tracker_server.c
#define _CRT_SECURE_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

#define MAX_PEERS 100
#define PORT 8888

typedef struct {
    char ip[INET_ADDRSTRLEN];
    int port;
} Peer;

Peer peers[MAX_PEERS];
int peer_count = 0;

void add_peer(const char* ip, int port) {
    if (peer_count < MAX_PEERS) {
        strcpy(peers[peer_count].ip, ip);
        peers[peer_count].port = port;
        peer_count++;
        printf("New peer added: %s:%d\n", ip, port);
    }
}

void send_peer_list(SOCKET client_socket) {
    send(client_socket, (char*)&peer_count, sizeof(peer_count), 0);
    send(client_socket, (char*)peers, sizeof(Peer) * peer_count, 0);
}

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr = { 0 };
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_socket, 5);

    printf("Tracker server started on port %d\n", PORT);

    while (1) {
        struct sockaddr_in client_addr;
        int client_addr_len = sizeof(client_addr);
        SOCKET client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);

        int port;
        recv(client_socket, (char*)&port, sizeof(port), 0);
        add_peer(client_ip, port);

        send_peer_list(client_socket);
        closesocket(client_socket);
    }

    closesocket(server_socket);
    WSACleanup();
    return 0;
}
