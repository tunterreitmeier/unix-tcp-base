#include <iostream>
#include "TCPListener/TCPListener.h"

void listenerHandle(TCPListener *server, int socket);

int main()
{
    TCPListener server("0.0.0.0", 3020, listenerHandle);
    server.run();
}

static const char response[] = "HTTP/1.1 101 Switching Protocols\n\
Upgrade : websocket\n\
Connection : Upgrade";

void listenerHandle(TCPListener *server, int socket)
{
    strlcpy(server->getBuffer(), response, server->bufferSize);
    server->sSend(socket);
}
