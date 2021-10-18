#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sstream>
#include <orbis/libkernel.h>

#include "util.h"
#include <graphics.h>

#include <thread>

// Dimensions
#define FRAME_WIDTH     1920
#define FRAME_HEIGHT    1080
#define FRAME_DEPTH        4

// Font information
#define FONT_SIZE   	   42

// Logging
// This is necessary, even if it isn't used
std::stringstream debugLogStream;


#define PORT 9025
// Use this for jailbreaking: https://github.com/0x199/ps4-ipi/blob/main/Internal%20PKG%20Installer/modules.cpp

std::stringstream screenTextStream;
std::mutex mtx;

#define DISP_TEXT(SIZE, ...) { \
        char * msg = new char[SIZE];\
        sprintf(msg, __VA_ARGS__);\
        mtx.lock();\
        screenTextStream << msg;\
        mtx.unlock();\
        sceKernelUsleep(10000);\
    }

#define NOTIFY_CONST(msg) {\
        system_notification(msg);\
    }

#define NOTIFY(SIZE, ...) {\
        char error[SIZE];\
        sprintf(error, __VA_ARGS__);\
        system_notification(error);\
    }

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
    NOTIFY(100, "Attempting to bind to bind!");
    while (bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) != 0) {
        sceKernelUsleep(20000);
    }
    NOTIFY(100, "Success!");

    // Listen and accept clients
    addrLen = sizeof(clientAddr);

    if (listen(sockfd, 5) != 0)
    {
        NOTIFY(50, "Failed to listen: %s\n", strerror(errno));
        return;
    }

    for (;;)
    {
        connfd = accept(sockfd, (struct sockaddr*)&clientAddr, &addrLen);

        if (connfd < 0)
        {
            DISP_TEXT(50, "Failed to accept client: %s\n", strerror(errno));
            return;
        }
        DISP_TEXT(32, "Accepted client: %d\n", connfd);
        // Write a "hello" message then terminate the connection
        const char msg[] = "hello\n";
        write(connfd, msg, sizeof(msg));
        close(connfd);
        DISP_TEXT(32,"Closed client %d\n", connfd);
    }
    close(sockfd);
}




int main(void)
{
    // No buffering
    setvbuf(stdout, NULL, _IONBF, 0);

    int rc;
    int video;
    int curFrame = 0;
    int frameID = 0;

    // Font faces
    FT_Face fontTxt;

    
    auto scene = new Scene2D(FRAME_WIDTH, FRAME_HEIGHT, FRAME_DEPTH);
    
    if(!scene->Init(0xC000000, 2))
    {
    	NOTIFY_CONST("Failed to initialize 2D scene");
    	for(;;);
    }
    // Background and foreground colors
    // Set colors
    Color bgColor = { 0, 0, 0 };
    Color fgColor = { 255, 255, 255 };


    // Initialize the font faces with arial (must be included in the package root!)
    const char *font = "/app0/assets/fonts/Gontserrat-Regular.ttf";

    if(!scene->InitFont(&fontTxt, font, FONT_SIZE))
    {
    	NOTIFY(100, "Failed to initialize font '%s'", font);
        for(;;);
    }

    std::thread t1(serverThread);
    for (;;)
    {
        scene->DrawText((char *)screenTextStream.str().c_str(), fontTxt, 150, 150, bgColor, fgColor);

        // Submit the frame buffer
        scene->SubmitFlip(frameID);
        scene->FrameWait(frameID);

        // Swap to the next buffer
        scene->FrameBufferSwap();
        frameID++;
    }
}
