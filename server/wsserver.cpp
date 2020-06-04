#include <iostream>
#include "TCPListener/WSListener.h"

void listenerHandle(TCPListener *server, int socket);

int main()
{
    WSListener server("0.0.0.0", 3020, listenerHandle);
    server.run();
}

void listenerHandle(TCPListener *server, int socket)
{
    server->sSend(socket);
}
