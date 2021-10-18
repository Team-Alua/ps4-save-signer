#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <orbis/libkernel.h>

#include "../../_common/log.h"

#define PORT 9025

// Logging
std::stringstream debugLogStream;

int main(void)
{
    int sockfd;
    int connfd;
    socklen_t addrLen;

    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;

    // No buffering
    setvbuf(stdout, NULL, _IONBF, 0);
    
    // Create a server socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
    {
        printf("Failed to create socket: %s\n", strerror(errno));
        for(;;);
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
        printf("Failed to listen: %s\n", strerror(errno));
        for(;;);
    }

    for (;;)
    {
        connfd = accept(sockfd, (struct sockaddr*)&clientAddr, &addrLen);

        if (connfd < 0)
        {
            printf("Failed to accept client: %s\n", strerror(errno));
            for(;;);
        }

        printf("Accepted client: %d", connfd);

        // Write a "hello" message then terminate the connection
        const char msg[] = "hello\n";
        write(connfd, msg, sizeof(msg));
        close(connfd);

        printf("Closed client %d\n", connfd);
        break;
    }
    close(sockfd);
    for(;;);
}
