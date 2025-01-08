P2P FILE TRANSFER 
----------------------------------

Overview:
---------
This project implements a basic peer-to-peer file transfer application in C, using the Windows Sockets (Winsock) API. 
Each peer can both listen for inbound connections (from other peers) and optionally connect to another peer’s IP and port. 
When connected, peers exchange files in both directions.

Features:
---------
1. Peer-to-Peer Communication:
   - Each instance can listen on a specified port for incoming connections.
   - It can also connect to a specified IP and port to initiate file transfer.

2. File Transfer:
   - If you provide a filename argument, the peer will send that file to the connected peer.
   - The peer also receives files from the other peer and saves them.

3. Usage:
   - Start one peer in "listening only" mode (no IP/port arguments).
   - Start another peer specifying the IP and port of the first peer, plus a filename to send.
   - They exchange files in both directions.


https://github.com/user-attachments/assets/f3bf26de-720b-4ef3-b189-26f675f3a01e

