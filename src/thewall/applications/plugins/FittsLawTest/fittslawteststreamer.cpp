#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>

int serversocket;

// return socket or -1 on error
int waitForConnection(int port);

// width, height, framerate, listening port

int main(int argc, char **argv) {

    if (argc < 5) {
        printf("width height frate port\n");
        exit(0);
    }

    int width = atoi(argv[1]);
    int height = atoi(argv[2]);
    int frate = atoi(argv[3]);
    int port = atoi(argv[4]);

    int streamsock = waitForConnection(port);
    if (streamsock == -1) {
        exit(0);
    }

    char *buffer = (char *)malloc(sizeof(char) * width * height * 3);

    int sent = 0;
    while(1) {
        sent = send(streamsock, buffer, width * height * 3, 0);
        if (sent == -1) {
            perror("send");
            break;
        }
        else if (sent == 0) {
            perror("send");
            break;
        }
    }

    printf("Streaming loop finished. Goodbye");

    free(buffer);

    close(streamsock);
    close(serversocket);

    return 0;
}


int waitForConnection(int port) {
    /* accept connection from sageStreamer */
    serversocket = ::socket(AF_INET, SOCK_STREAM, 0);
    if ( serversocket == -1 ) {
            printf("%s() : couldn't create socket", __FUNCTION__);
            return -1;
    }

    // setsockopt
    int optval = 1;
    if ( setsockopt(serversocket, SOL_SOCKET, SO_REUSEADDR, &optval, (socklen_t)sizeof(optval)) != 0 ) {
            printf("%s() : setsockopt SO_REUSEADDR failed",  __FUNCTION__);
    }

    // bind to port
    struct sockaddr_in localAddr, clientAddr;
    memset(&localAddr, 0, sizeof(localAddr));
    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    localAddr.sin_port = htons(port);

    // bind
    if( bind(serversocket, (struct sockaddr *)&localAddr, sizeof(struct sockaddr_in)) != 0) {
            printf("%s() : bind error",  __FUNCTION__);
            return -1;
    }

    // put in listen mode
    listen(serversocket, 15);

    // accept
    /** accept will BLOCK **/
//	qDebug() << "SN_SageStreamWidget::waitForPixelStreamerConn() : sageappid" << _sageAppId << "Before accept(). TCP port" << protocol+port << QTime::currentTime().toString("hh:mm:ss.zzz");

    memset(&clientAddr, 0, sizeof(clientAddr));
    int addrLen = sizeof(struct sockaddr_in);

    int streamsocket = -1;
    if ((streamsocket = accept(serversocket, (struct sockaddr *)&clientAddr, (socklen_t*)&addrLen)) == -1) {
            printf("%s() : accept error", __FUNCTION__);
            perror("accept");
            return -1;
    }
    else {
        printf("A receiver has connected !\n");
    }

    return streamsocket;
}
