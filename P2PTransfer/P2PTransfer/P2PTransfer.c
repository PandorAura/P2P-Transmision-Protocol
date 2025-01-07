#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdint.h>
#include <assert.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

#define TCP_PORT 12345
#define UDP_PORT 12346
#define CHUNK_SIZE 4096
#define BUFFER_SIZE 1024
#define MAX_PEERS 10

// Peer Structure to store IP and Port
typedef struct {
    char ip[INET_ADDRSTRLEN];
    uint16_t port;
} Peer;

Peer peers[MAX_PEERS];
int peer_count = 0;

// Initialize Winsock
int init_winsock() {
    WSADATA wsa_data;
    return WSAStartup(MAKEWORD(2, 2), &wsa_data);
}

// Helper to add a new peer
void add_peer(const char* ip, uint16_t port) {
    if (peer_count < MAX_PEERS) {
        strcpy_s(peers[peer_count].ip, sizeof(peers[peer_count].ip), ip);
        peers[peer_count].port = port;
        peer_count++;
    }
}

// Send file metadata (size, chunks, etc.)
void send_metadata_request(SOCKET tcp_sock, const char* file_name, struct sockaddr_in* peer_addr) {
    // Send metadata request over TCP
    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "METADATA_REQ %s", file_name);
    sendto(tcp_sock, buffer, strlen(buffer), 0, (struct sockaddr*)peer_addr, sizeof(*peer_addr));
}

// Request a file chunk over UDP
void request_file_chunk(SOCKET udp_sock, const char* file_name, uint64_t chunk_number, struct sockaddr_in* peer_addr) {
    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "CHUNK_REQ %s %llu", file_name, chunk_number);
    sendto(udp_sock, buffer, strlen(buffer), 0, (struct sockaddr*)peer_addr, sizeof(*peer_addr));
}

// Receive file chunk over UDP
void receive_file_chunk(SOCKET udp_sock, uint8_t* chunk_buffer) {
    struct sockaddr_in peer_addr;
    int peer_addr_len = sizeof(peer_addr);
    int bytes_received = recvfrom(udp_sock, (char*)chunk_buffer, CHUNK_SIZE, 0, (struct sockaddr*)&peer_addr, &peer_addr_len);
    if (bytes_received == SOCKET_ERROR) {
        printf("Error receiving file chunk\n");
        return;
    }
    printf("Received chunk of size %d\n", bytes_received);
}

// Main server-side functionality
DWORD WINAPI server_side(LPVOID lpParam) {
    SOCKET tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
    SOCKET udp_sock = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(TCP_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(tcp_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("TCP Bind failed: %d\n", WSAGetLastError());
        return 1;
    }

    if (bind(udp_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("UDP Bind failed: %d\n", WSAGetLastError());
        return 1;
    }

    listen(tcp_sock, 5);
    printf("Server listening for connections...\n");

    while (1) {
        struct sockaddr_in client_addr;
        int client_addr_len = sizeof(client_addr);
        SOCKET client_sock = accept(tcp_sock, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_sock == INVALID_SOCKET) {
            printf("Accept failed: %d\n", WSAGetLastError());
            continue;
        }

        // Process incoming peer discovery or file metadata requests here...
        printf("Server accepted connection\n");
    }

    return 0;
}

// Main client-side functionality
DWORD WINAPI client_side(LPVOID lpParam) {
    const char* peer_ip = "127.0.0.1";
    uint16_t peer_port = TCP_PORT;
    const char* file_name = "test_file.txt";

    printf("Client starting...\n");

    SOCKET tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
    SOCKET udp_sock = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(peer_port);
    inet_pton(AF_INET, peer_ip, &server_addr.sin_addr);

    if (connect(tcp_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Connection failed: %d\n", WSAGetLastError());
        return 1;
    }

    // Send metadata request for a file
    send_metadata_request(tcp_sock, file_name, &server_addr);

    // Request file chunks over UDP
    uint64_t chunk_number = 0;
    uint8_t chunk_buffer[CHUNK_SIZE];
    while (1) {
        request_file_chunk(udp_sock, file_name, chunk_number, &server_addr);
        receive_file_chunk(udp_sock, chunk_buffer);
        chunk_number++;
    }

    return 0;
}

// Main function
int main() {
    if (init_winsock() != 0) {
        printf("Winsock initialization failed\n");
        return 1;
    }

    // Create threads for server and client
    HANDLE server_thread, client_thread;
    server_thread = CreateThread(NULL, 0, server_side, NULL, 0, NULL);

    // Sleep briefly to give the server a chance to start before the client attempts to connect
    Sleep(1000);  // 1-second delay

    client_thread = CreateThread(NULL, 0, client_side, NULL, 0, NULL);

    // Wait for both threads to finish
    WaitForSingleObject(server_thread, INFINITE);
    WaitForSingleObject(client_thread, INFINITE);

    WSACleanup();
    return 0;
}
