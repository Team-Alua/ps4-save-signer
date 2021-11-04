#include "server.h"

#define PORT 9025


void serverThread() {
    int sockfd;
    int connfd;
    socklen_t addrLen;

    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;

        // Create a server socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
    {
        NOTIFY(50, "Failed to create socket: %s\n", strerror(errno));
        return;
    }

    // Bind to 0.0.0.0:PORT
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(PORT);

    // Fixes issue where it would not accept the same address after it was closed
    while (bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) != 0) {
        sceKernelUsleep(20000);
    }


    // Listen and accept clients
    addrLen = sizeof(clientAddr);

    if (listen(sockfd, 5) != 0)
    {
        NOTIFY(50, "Failed to listen: %s\n", strerror(errno));
        return;
    }

    
    NOTIFY_CONST("Now listening...\n");
    for (;;)
    {
        connfd = accept(sockfd, (struct sockaddr*)&clientAddr, &addrLen);

        if (connfd < 0)
        {

            debugLogStream <<  "Failed to accept client: " << strerror(errno) << "\n";
            return;
        }
        debugLogStream <<  "Accepted client: " << connfd << "\n";

        // Write a "hello" message then terminate the connection
        const char msg[] = "hello\n";
        write(connfd, msg, sizeof(msg));
        close(connfd);
        debugLogStream <<  "Closed client: " << connfd << "\n";
    }
}