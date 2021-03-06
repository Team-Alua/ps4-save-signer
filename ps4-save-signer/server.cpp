#include "server.hpp"
#include "cmd_utils.hpp"
#include "cmd_constants.hpp"
#include "cmd_uploadfile.hpp"
#include "cmd_savegen.hpp"
#include "cmd_saveextract.hpp"
#include "cmd_deleteupload.hpp"
#include "cmd_resign.hpp"
#include "signal.h"
#define PORT 9025


void clientHandler(int connfd);

void serverThread() {
    // Don't crash when writing to a closed connection
    signal(SIGPIPE, SIG_IGN);

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
    for (;;) {
        connfd = accept(sockfd, (struct sockaddr*)&clientAddr, &addrLen);
        if (connfd < 0) {
            return;
        }
        std::thread clientThread(clientHandler, connfd);
        clientThread.detach();
    }
}


void clientHandler(int connfd) {
    sendStatusCode(connfd, CMD_STATUS_READY);
    Log("Client connected!");
    while (true) {
        PacketHeader pHeader;

        if (getPacketHeader(connfd, &pHeader) != CMD_STATUS_READY) {
            break;
        }

        switch (pHeader.cmd) {
            case CMD_UPLOAD_FILE: {
                handleUploadFile(connfd, &pHeader);
                break;
            }
            case CMD_SAVE_GEN: {
                handleSaveGenerating(connfd, &pHeader);
                break;
            }
            case CMD_SAVE_EXTRACT: {
                handleSaveExtract(connfd, &pHeader);
                break;
            }
            case CMD_SAVE_RESIGN: {
                handleSaveResign(connfd, &pHeader);
                break;
            }
            case CMD_DELETE_UPLOAD: {
                handleUploadDelete(connfd, &pHeader);
                break;
            }
            default: {
                sendStatusCode(connfd, CMD_IS_INVALID);
                break;
            }

        }
    }
    close(connfd);
    Log("Client disconnected!");
}