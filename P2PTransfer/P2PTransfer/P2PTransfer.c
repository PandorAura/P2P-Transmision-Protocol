#define _CRT_SECURE_NO_WARNINGS

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <stdbool.h>

// Link against ws2_32.lib
#pragma comment(lib, "ws2_32.lib")

#define CHUNK_SIZE 4096

void cleanup(SOCKET sock) {
    closesocket(sock);
    WSACleanup();
}

// Helper to initialize Winsock
int init_winsock() {
    WSADATA wsa;
    int result = WSAStartup(MAKEWORD(2, 2), &wsa);
    if (result != 0) {
        printf("WSAStartup failed with error: %d\n", result);
    }
    return result;
}

// Function to send a file
void send_file(SOCKET peer_socket, const char* filename) {
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        printf("Cannot open file '%s'\n", filename);
        return;
    }

    // Send the filename length and filename
    int filename_length = strlen(filename) + 1;
    send(peer_socket, (char*)&filename_length, sizeof(filename_length), 0);
    send(peer_socket, filename, filename_length, 0);

    // Send the file size
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    send(peer_socket, (char*)&file_size, sizeof(file_size), 0);

    // Send file content in chunks
    char buffer[CHUNK_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, CHUNK_SIZE, fp)) > 0) {
        send(peer_socket, buffer, bytes_read, 0);
    }

    printf("File '%s' sent successfully.\n", filename);
    fclose(fp);
}

// Function to receive a file
void receive_file(SOCKET peer_socket) {
    // Receive the filename length and filename
    int filename_length;
    recv(peer_socket, (char*)&filename_length, sizeof(filename_length), 0);
    char filename[256];
    recv(peer_socket, filename, filename_length, 0);

    // Open the file for writing
    FILE* fp = fopen(filename, "wb");
    if (!fp) {
        printf("Cannot create file '%s'\n", filename);
        return;
    }

    // Receive the file size
    long file_size;
    recv(peer_socket, (char*)&file_size, sizeof(file_size), 0);

    // Receive the file content
    char buffer[CHUNK_SIZE];
    long total_received = 0;
    int bytes_received;
    while (total_received < file_size &&
        (bytes_received = recv(peer_socket, buffer, CHUNK_SIZE, 0)) > 0) {
        fwrite(buffer, 1, bytes_received, fp);
        total_received += bytes_received;
    }

    printf("File '%s' received successfully.\n", filename);
    fclose(fp);
}

// Main peer-to-peer logic
void p2p_peer(const char* listen_port, const char* connect_ip, const char* connect_port, const char* filename) {
    // Start listening
    SOCKET listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in listen_addr = { 0 };
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_port = htons(atoi(listen_port));
    listen_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listen_socket, (struct sockaddr*)&listen_addr, sizeof(listen_addr)) < 0) {
        printf("Bind failed with error code: %d\n", WSAGetLastError());
        cleanup(listen_socket);
        return;
    }
    listen(listen_socket, 1);

    printf("Listening on port %s...\n", listen_port);

    // Attempt to connect to another peer if IP and port are provided
    SOCKET connect_socket = INVALID_SOCKET;
    if (connect_ip && connect_port) {
        connect_socket = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in peer_addr = { 0 };
        peer_addr.sin_family = AF_INET;
        peer_addr.sin_port = htons(atoi(connect_port));
        inet_pton(AF_INET, connect_ip, &peer_addr.sin_addr);

        if (connect(connect_socket, (struct sockaddr*)&peer_addr, sizeof(peer_addr)) == 0) {
            printf("Connected to peer %s:%s\n", connect_ip, connect_port);
            send_file(connect_socket, filename);
            receive_file(connect_socket);
        }
        else {
            printf("Failed to connect to peer.\n");
        }
    }

    // Accept incoming connections
    struct sockaddr_in peer_addr;
    int peer_addr_len = sizeof(peer_addr);
    SOCKET peer_socket = accept(listen_socket, (struct sockaddr*)&peer_addr, &peer_addr_len);

    if (peer_socket != INVALID_SOCKET) {
        printf("Peer connected.\n");
        receive_file(peer_socket);
        send_file(peer_socket, filename);
    }

    cleanup(listen_socket);
    if (connect_socket != INVALID_SOCKET) cleanup(connect_socket);
}

int main(int argc, char* argv[]) {
    if (init_winsock() != 0) return 1;

    if (argc < 2) {
        printf("Usage:\n");
        printf("  %s <listen_port> [peer_ip] [peer_port] [file_to_send]\n", argv[0]);
        WSACleanup();
        return 1;
    }

    const char* listen_port = argv[1];
    const char* connect_ip = argc > 3 ? argv[2] : NULL;
    const char* connect_port = argc > 3 ? argv[3] : NULL;
    const char* filename = argc > 4 ? argv[4] : "received_file.txt";

    p2p_peer(listen_port, connect_ip, connect_port, filename);

    WSACleanup();
    return 0;
}