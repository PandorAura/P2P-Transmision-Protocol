#define _CRT_SECURE_NO_WARNINGS

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

#define CHUNK_SIZE 4096
#define TRACKER_IP "192.168.0.92"  // Updated to use the specified IP
#define TRACKER_PORT 8888

typedef struct {
    char ip[INET_ADDRSTRLEN];
    int port;
} Peer;

// Helper to initialize Winsock
int init_winsock() {
    WSADATA wsa;
    int result = WSAStartup(MAKEWORD(2, 2), &wsa);
    return result;
}

// Register the peer with the tracker and fetch the peer list
int register_with_tracker(const char* listen_port, Peer* peer_list, int* peer_count) {
    SOCKET tracker_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in tracker_addr = { 0 };
    tracker_addr.sin_family = AF_INET;
    tracker_addr.sin_port = htons(TRACKER_PORT);
    inet_pton(AF_INET, TRACKER_IP, &tracker_addr.sin_addr);

    if (connect(tracker_socket, (struct sockaddr*)&tracker_addr, sizeof(tracker_addr)) != 0) {
        printf("Failed to connect to tracker\n");
        return -1;
    }

    int port = atoi(listen_port);
    send(tracker_socket, (char*)&port, sizeof(port), 0);

    recv(tracker_socket, (char*)peer_count, sizeof(*peer_count), 0);
    recv(tracker_socket, (char*)peer_list, sizeof(Peer) * (*peer_count), 0);

    closesocket(tracker_socket);
    return 0;
}

// Send file
void send_file(SOCKET peer_socket, const char* filename) {
    FILE* fp = fopen(filename, "rb");
    char buffer[CHUNK_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, CHUNK_SIZE, fp)) > 0) {
        send(peer_socket, buffer, bytes_read, 0);
    }
    fclose(fp);
}

// Receive file
void receive_file(SOCKET peer_socket) {
    FILE* fp = fopen("received_file.txt", "wb");
    char buffer[CHUNK_SIZE];
    int bytes_received;
    while ((bytes_received = recv(peer_socket, buffer, CHUNK_SIZE, 0)) > 0) {
        fwrite(buffer, 1, bytes_received, fp);
    }
    fclose(fp);
}

// Main P2P peer logic
void p2p_peer(const char* listen_port, const char* filename) {
    Peer peer_list[100];
    int peer_count = 0;

    register_with_tracker(listen_port, peer_list, &peer_count);

    SOCKET listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in listen_addr = { 0 };
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_port = htons(atoi(listen_port));
    listen_addr.sin_addr.s_addr = INADDR_ANY;

    bind(listen_socket, (struct sockaddr*)&listen_addr, sizeof(listen_addr));
    listen(listen_socket, 5);

    printf("Listening on port %s. Found %d peers.\n", listen_port, peer_count);

    // Try to connect to discovered peers
    for (int i = 0; i < peer_count; ++i) {
        printf("Trying to connect to %s:%d\n", peer_list[i].ip, peer_list[i].port);
        SOCKET peer_socket = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in peer_addr = { 0 };
        peer_addr.sin_family = AF_INET;
        peer_addr.sin_port = htons(peer_list[i].port);
        inet_pton(AF_INET, peer_list[i].ip, &peer_addr.sin_addr);

        if (connect(peer_socket, (struct sockaddr*)&peer_addr, sizeof(peer_addr)) == 0) {
            printf("Connected to peer %s:%d\n", peer_list[i].ip, peer_list[i].port);
            send_file(peer_socket, filename);
            receive_file(peer_socket);
            closesocket(peer_socket);
        }
    }

    SOCKET peer_socket = accept(listen_socket, NULL, NULL);
    if (peer_socket != INVALID_SOCKET) {
        receive_file(peer_socket);
        send_file(peer_socket, filename);
    }

    closesocket(listen_socket);
    WSACleanup();
}

int main(int argc, char* argv[]) {
    if (init_winsock() != 0) return 1;

    if (argc < 3) {
        printf("Usage: %s <listen_port> <file_to_send>\n", argv[0]);
        return 1;
    }

    const char* listen_port = argv[1];
    const char* filename = argv[2];

    p2p_peer(listen_port, filename);

    return 0;
}
