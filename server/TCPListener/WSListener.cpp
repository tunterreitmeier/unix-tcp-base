#include <sys/select.h>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include "WSListener.h"

std::ostream &operator<<(std::ostream &os, Header &header)
{
    os << header.name << ": " << header.content << std::endl;
    return os;
}

const char WSListener::wsguid[37] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
const std::string WSListener::handshakePre = "HTTP/1.1 101 Switching Protocols\r\n\
Upgrade: websocket\r\n\
Connection: Upgrade\r\n\
Sec-WebSocket-Accept: ";

WSListener::WSListener(const char *ip, uint16_t port, CallbackHandle callback)
    : TCPListener(ip, port, callback) {}

void WSListener::getHeaders(const std::string &data)
{
    headers.clear();
    std::istringstream iss(data);
    std::string line;
    std::size_t pos;
    while (getline(iss, line))
    {
        pos = line.find(": ");
        if (pos != std::string::npos)
        {
            headers.push_back({line.substr(0, pos), line.substr(pos + 2, line.length() - pos - 3)});
        }
    }
}

bool WSListener::hasHeader(const std::string &name)
{
    for (Header &header : headers)
    {
        if (name.compare(header.name) == 0)
            return true;
    }
    return false;
}

Header &WSListener::getHeader(const std::string &name)
{
    for (Header &header : headers)
    {
        if (name.compare(header.name) == 0)
            return header;
    }
}

void WSListener::run()
{
    struct sockaddr_in clientname;
    socklen_t size;

    int tcpSocket = createSocket();
    if (tcpSocket == -1)
        return;

    fd_set master;
    FD_ZERO(&master);

    FD_SET(tcpSocket, &master);

    while (true)
    {
        fd_set copy = master;
        if (select(FD_SETSIZE, &copy, NULL, NULL, NULL) < 0)
        {
            perror("select");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < FD_SETSIZE; i++)
        {
            if (FD_ISSET(i, &copy))
            {
                if (i == tcpSocket) // new socket
                {
                    std::cout << "Waiting for new connection..." << std::endl;
                    size = sizeof(clientname);
                    int clientSocket = accept(tcpSocket, (sockaddr *)&clientname, &size);
                    std::cout << "New client: " << clientSocket << std::endl;
                    if (!handleHandshake(clientSocket))
                    {
                        std::cerr << "Error making correct handshake" << std::endl;
                        shutdown(clientSocket, SHUT_RDWR);
                    }
                    FD_SET(clientSocket, &master);
                }
                else // existing socket
                {
                    memset(buffer, 0, bufferSize);
                    ssize_t bytesReceived = recv(i, buffer, bufferSize, 0);
                    if (bytesReceived == -1)
                    {
                        std::cerr << "Error receiving client data: " << errno << std::endl;
                        shutdown(i, SHUT_RDWR);
                        FD_CLR(i, &master);
                    }
                    if (bytesReceived == 0)
                    {
                        std::cout << "Client disconnected" << std::endl;
                        shutdown(i, SHUT_RDWR);
                        FD_CLR(i, &master);
                    }

                    // puts decoded message in payload
                    decodeFrame();

                    // broadcast: send to all open sockets that are not server and not self
                    for (int out = 0; out < FD_SETSIZE; out++)
                    {
                        if (FD_ISSET(out, &master))
                        {
                            if (out != tcpSocket)
                            {
                                std::cout << "Sending to " << out << std::endl;
                                sendFrame(out);
                            }
                        }
                    }
                }
            }
        }
    }
    shutdown(tcpSocket, SHUT_RDWR);
}

bool WSListener::handleHandshake(int socket)
{
    memset(buffer, 0, bufferSize);
    ssize_t bytesReceived = recv(socket, buffer, bufferSize, 0);
    if (bytesReceived == -1)
    {
        std::cerr << "Error receiving client data: " << errno << std::endl;
        return false;
    }
    std::cout << "Websocket GET received" << std::endl;

    getHeaders(buffer);

    if (hasHeader("Sec-WebSocket-Key"))
    {
        // get handshake key from header
        std::string key64 = getHeader("Sec-WebSocket-Key").content;
        rtrim(key64);

        // extend raw string with fixed guid
        unsigned char keyExt[100];
        memset(keyExt, 0, 0);
        strcpy((char *)keyExt, key64.c_str());
        strcat((char *)keyExt, wsguid);

        // SHA1 Hash the extended string
        unsigned char keySHA1[21];
        memset(keySHA1, 0, 21);
        SHA1(keyExt, strlen((char *)keyExt), keySHA1);

        // base64 encode the sha1 hash
        char wsKey[50];
        memset(wsKey, 0, 50);
        b64encode(keySHA1, strlen((char *)keySHA1), wsKey, 50);

        // put handshake response in buffer and send
        memset(buffer, 0, bufferSize);
        strcpy(buffer, handshakePre.c_str());
        strcat(buffer, wsKey);
        strcat(buffer, "\r\n\r\n");

        std::cout << "Sending handshake: " << wsKey << std::endl;
        ssize_t sendResult = send(socket, buffer, strlen(buffer), 0);
        return sendResult != -1;
    }
    else
    {
        std::cerr << "Key header not found - cannot handshake" << std::endl;
        return false;
    }
}

void WSListener::decodeFrame()
{
    // mask is 4 bytes
    char mask[5];
    for (int i = 0; i < 4; i++)
        mask[i] = buffer[i + 2];
    mask[4] = '\0';
    uint8_t inLength = (uint8_t)(buffer[1] - 128);
    memset(payloadBuffer, 0, inLength);
    for (uint8_t i = 0; i < inLength; i++)
    {
        payloadBuffer[i] = buffer[i + 6] ^ mask[i % 4];
    }
    payloadBuffer[inLength] = '\0';
    std::cout << "Rcv: " << payloadBuffer << std::endl;
}

void WSListener::sendFrame(int socket)
{
    memset(buffer, 0, bufferSize);
    // frame the websocket message according to spec
    // 1000 0001 00000006   - 2 bytes
    buffer[0] = (char)129;             // raw binary: 1000 0001
    buffer[1] = strlen(payloadBuffer); // payload length in bit with leading 0 - unmasked
    strcat(buffer, payloadBuffer);
    send(socket, buffer, strlen(buffer), 0);
}


int WSListener::b64encode(const unsigned char *in, int in_len, char *out, int out_len)
{
    int ret = 0;
    BIO *b64 = BIO_new(BIO_f_base64());
    BIO *bio = BIO_new(BIO_s_mem());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_push(b64, bio);
    ret = BIO_write(b64, in, in_len);
    BIO_flush(b64);
    if (ret > 0)
    {
        ret = BIO_read(bio, out, out_len);
    }
    BIO_free(b64);
    return ret;
}

int WSListener::b64decode(const unsigned char *in, int in_len, char *out, int out_len)
{
    int ret = 0;
    BIO *b64 = BIO_new(BIO_f_base64());
    BIO *bio = BIO_new(BIO_s_mem());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_push(b64, bio);
    ret = BIO_write(bio, in, in_len);
    BIO_flush(bio);
    if (ret)
    {
        ret = BIO_read(b64, out, out_len);
    }

    BIO_free(b64);
    return ret;
}

const std::string WHITESPACE = " \n\r\t\f\v";
std::string rtrim(const std::string &s)
{
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}
