//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <winsock2.h>
//#include <ws2tcpip.h>
//
//#pragma comment(lib, "ws2_32.lib")
//
//#define TCP_PORT 12345
//#define BUFFER_SIZE 1024
//
//int main() {
//    WSADATA wsa_data;
//    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
//        printf("WSAStartup failed: %d\n", WSAGetLastError());
//        return 1;
//    }
//
//    // Create TCP socket
//    SOCKET client_sock = socket(AF_INET, SOCK_STREAM, 0);
//    if (client_sock == INVALID_SOCKET) {
//        printf("Socket creation failed: %d\n", WSAGetLastError());
//        WSACleanup();
//        return 1;
//    }
//
//    struct sockaddr_in server_addr = { 0 };
//    server_addr.sin_family = AF_INET;
//    server_addr.sin_port = htons(TCP_PORT);
//    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr); // Replace with server IP for WAN
//
//    if (connect(client_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
//        printf("Connect failed: %d\n", WSAGetLastError());
//        closesocket(client_sock);
//        WSACleanup();
//        return 1;
//    }
//
//    // Send PEER_DISCOVERY request
//    char request[] = "PEER_DISCOVERY";
//    send(client_sock, request, strlen(request), 0);
//
//    // Receive response
//    char response[BUFFER_SIZE];
//    int recv_len = recv(client_sock, response, sizeof(response), 0);
//    if (recv_len > 0) {
//        response[recv_len] = '\0';
//        printf("Response from server: %s\n", response);
//    }
//    else {
//        printf("Receive failed: %d\n", WSAGetLastError());
//    }
//
//    closesocket(client_sock);
//    WSACleanup();
//    return 0;
//}
