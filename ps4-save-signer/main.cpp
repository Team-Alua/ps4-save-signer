#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "util.h"
#include <orbis/libkernel.h>

#define PORT 9025
// Use this for jailbreaking: https://github.com/0x199/ps4-ipi/blob/main/Internal%20PKG%20Installer/modules.cpp
#define SYS_PRINT(SIZE, ...) { char error[SIZE]; sprintf(error, __VA_ARGS__); system_notification(error);  }




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
        
        SYS_PRINT(50, "Failed to create socket: %s\n", strerror(errno));
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
        SYS_PRINT(50, "Failed to listen: %s\n", strerror(errno));
        for(;;);
    }

    for (;;)
    {
        connfd = accept(sockfd, (struct sockaddr*)&clientAddr, &addrLen);

        if (connfd < 0)
        {
            SYS_PRINT(50, "Failed to accept client: %s\n", strerror(errno));
            for(;;);
        }

        SYS_PRINT(32, "Accepted client: %d", connfd);

        // Write a "hello" message then terminate the connection
        const char msg[] = "hello\n";
        write(connfd, msg, sizeof(msg));
        close(connfd);

        SYS_PRINT(32,"Closed client %d\n", connfd);
        break;
    }
    close(sockfd);
    for(;;);
}
