#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

// Idea: define type for socket connections instead of int

int main()
{
    const int port = 3020;
    const char ip[] = "0.0.0.0";
    unsigned int chunkSize = 4096;

    // create socket
    static int tcpSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcpSocket == -1)
    {
        std::cerr << "Socket creation failed" << std::endl;
        return 1;
    }

    // catch premature exit
    signal(SIGINT, [](int signum) {
        std::cout << "Interrupt signal (" << signum << ") received.\n";
        shutdown(tcpSocket, SHUT_RDWR);
        exit(signum);
    });

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
    while (true)
    {
        memset(buffer, 0, chunkSize);
        ssize_t bytesReceived = recv(clientSocket, buffer, chunkSize, 0);
        if (bytesReceived == -1)
        {
            std::cerr << "Error receiving client data: " << errno << std::endl;
            break;
        }
        if (bytesReceived == 0)
        {
            std::cout << "Client disconnected" << std::endl;
            break;
        }
        std::cout << "Received data: " << buffer << std::endl;
        strlcpy(buffer, "thanks", chunkSize);
        send(clientSocket, buffer, chunkSize, 0);
    }

    // close sockets
    shutdown(clientSocket, SHUT_RDWR);

    shutdown(tcpSocket, SHUT_RDWR);
}
