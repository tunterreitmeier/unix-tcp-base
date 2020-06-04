#include "TCPListener.h"

TCPListener::TCPListener(const char *ip, uint16_t port, CallbackHandle callback)
    : port(port), callback(callback)
{
    this->ip = new char[16];
    strlcpy(this->ip, ip, 16);
}

TCPListener::~TCPListener()
{
    delete ip;
    cleanup();
}
void TCPListener::cleanup()
{
    // shutdown(tcpSocket, SHUT_RDWR);
}

bool TCPListener::init()
{
    return true;
}

int TCPListener::createSocket()
{
    int tcpSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcpSocket == -1)
    {
        std::cerr << "Error creating local socket: " << tcpSocket << " - " << errno << std::endl;
        return tcpSocket;
    }
    sockaddr_in hint;
    hint.sin_family = AF_INET;
    // hint.sin_addr.s_addr = INADDR_ANY; // listen on all
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
        return -1;
    }
    return tcpSocket;
}

int TCPListener::waitForConnection(int tcpSocket)
{
    return accept(tcpSocket, NULL, NULL);
}

void TCPListener::run()
{
    int tcpSocket = createSocket();
    if (tcpSocket == -1) return;
    int clientSocket = waitForConnection(tcpSocket);
    while (true)
    {
        memset(buffer, 0, bufferSize);
        ssize_t bytesReceived = recv(clientSocket, buffer, bufferSize, 0);
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
        callback(this, clientSocket);
        // strlcpy(buffer, "thanks", chunkSize);
        //send(clientSocket, buffer, chunkSize, 0);
    }
    shutdown(clientSocket, SHUT_RDWR);
    shutdown(tcpSocket, SHUT_RDWR);
}

void TCPListener::sSend(int socket)
{
    buffer[bufferSize - 1] = 0;
    send(socket, buffer, bufferSize, 0);
}

char * TCPListener::getBuffer()
{
    return (char *) buffer;
}
