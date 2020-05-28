#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

int main()
{
    const int port = 3020;
    const char serverIp[] = "192.168.178.79";
    unsigned int chunkSize = 4096;

    // create tcp socket
    int tcpSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcpSocket == -1)
    {
        std::cerr << "Socket creation failed" << std::endl;
        return -1;
    }

    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(port);
    inet_pton(AF_INET, serverIp, &hint.sin_addr); // binary ip address from string

    // connect to server
    if (connect(tcpSocket, (sockaddr *)&hint, sizeof(hint)) != 0)
    {
        std::cerr << "Error connecting to server" << std::endl;
        std::cerr << errno << std::endl;
        return -2;
    }

    char buffer[chunkSize];
    system("clear");
    std::cout << "Connected to Server on port " << port << std::endl;
    std::cout << "Enter 'quit' to exit" << std::endl;

    // loop with blocking function calls - break on error or user quit
    while (true)
    {
        // send data
        memset(buffer, 0, chunkSize);
        std::cout << "> ";
        std::cin >> buffer;
        if (std::strcmp(buffer, "quit") == 0)
        {
            std::cout << "Bye" << std::endl;
            break;
        }
        ssize_t sendStatus = send(tcpSocket, buffer, chunkSize, 0);
        if (sendStatus == -1)
        {
            std::cout << "Message was not sent " << sendStatus << errno << std::endl;
            continue;
        }

        // receive response
        memset(buffer, 0, chunkSize);
        ssize_t bytesReceived = recv(tcpSocket, buffer, chunkSize, 0);
        std::cout << "Received data: " << buffer << std::endl;
        if (bytesReceived == -1)
        {
            std::cerr << "Error receiving server data: " << errno << std::endl;
            break;
        }
        if (bytesReceived == 0)
        { //gracefully closed
            std::cout << "Server shut down" << std::endl;
            break;
        }
    }
    shutdown(tcpSocket, SHUT_RDWR);
}
