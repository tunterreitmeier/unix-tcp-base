#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

// Idea: define type for socket connections instead of int

void getTextResponse(char *buffer, uint chunkSize);

int main()
{
    const int port = 3020;
    const char ip[] = "0.0.0.0";
    uint chunkSize = 4096;

    // create socket
    int tcpSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcpSocket == -1)
    {
        std::cerr << "Socket creation failed" << std::endl;
        return 1;
    }

    // bind listener to port
    sockaddr_in hint;
    hint.sin_family = AF_INET;
    // hint.sin_addr.s_addr = INADDR_ANY;
    hint.sin_port = htons(port);            // unsigned short big endian
    inet_pton(AF_INET, ip, &hint.sin_addr); // binary ip address from string

    if (bind(tcpSocket, (sockaddr *)&hint, sizeof(hint)) != 0)
    {
        std::cerr << "Error binding:" << errno << std::endl;
        return -1;
    }
    if (listen(tcpSocket, SOMAXCONN) != 0)
    {
        std::cerr << "Error listening:" << errno << std::endl;
        return -2;
    }

    system("clear");
    std::cout << "Listening on port " << port << std::endl;

    // open client socket
    sockaddr_in client;
    socklen_t clientSize = sizeof(client);

    char host[NI_MAXHOST];
    char service[NI_MAXSERV];
    memset(host, 0, NI_MAXHOST);
    memset(service, 0, NI_MAXSERV);

    int clientSocket = accept(tcpSocket, (sockaddr *)&client, &clientSize);

    // handle client data
    int nameInfo = getnameinfo((sockaddr *)&client, clientSize, host, NI_MAXHOST, service, NI_MAXSERV, 0); // get client data and write to mem
    if (nameInfo != 0)
    {
        inet_ntop(AF_INET, &hint.sin_addr, host, clientSize);
    }
    std::cout << "Connection from " << host << " on port " << service << std::endl;

    char buffer[chunkSize];

    memset(buffer, 0, chunkSize);
    ssize_t bytesReceived = recv(clientSocket, buffer, chunkSize, 0);
    if (bytesReceived == -1)
    {
        std::cerr << "Error receiving client data: " << errno << std::endl;
    }
    if (bytesReceived == 0)
    {
        std::cout << "Client disconnected" << std::endl;
    }
    std::cout << "Received data: " << buffer << std::endl;
    getTextResponse(buffer, chunkSize);
    std::cout << "Response: " << buffer << std::endl;
    send(clientSocket, buffer, chunkSize, 0);

    // close sockets
    shutdown(clientSocket, SHUT_RDWR);

    shutdown(tcpSocket, SHUT_RDWR);
    std::cout << "Sockets closed" << std::endl;
}

void getTextResponse(char *buffer, uint bufferSize)
{
    strlcpy(buffer, "HTTP/1.1 200 OK\n\
Date: Thu, 28 May 2020 21:49:27 GMT\n\
Server: Native Server (Unix)\n\
Content-Length: 75\n\
Connection: close\n\
Content-Type: text/html; charset=UTF-8\n\
\n\
\n\
<html><body><h1>It works!</h1><p>C++ Static Text Server</p></body></html>",
            bufferSize);
    if (buffer[bufferSize - 1] != '\0')
        buffer[bufferSize - 1] = '\0';
}
