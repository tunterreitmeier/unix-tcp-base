COMPILER = c++
CFLAGS = -std=c++11

server2: tcplistener.o
	$(COMPILER) $(CFLAGS) ./server/server2.cpp ./server/TCPListener/TCPListener.o -o ./server/server2

wsserver: tcplistener.o wslistener.o
	$(COMPILER) $(CFLAGS) -lssl -lcrypto -L/usr/local/opt/openssl/lib -I/usr/local/opt/openssl/include ./server/wsserver.cpp ./server/TCPListener/TCPListener.o ./server/TCPListener/WSListener.o -o ./server/wsserver

tcplistener.o:
	$(COMPILER) $(CFLAGS) -c ./server/TCPListener/TCPListener.cpp -o ./server/TCPListener/TCPListener.o

wslistener.o:
	$(COMPILER) $(CFLAGS) -c ./server/TCPListener/WSListener.cpp -o ./server/TCPListener/WSListener.o

server:
	$(COMPILER) $(CFLAGS) ./server/server.cpp -o ./server/server

client:
	$(COMPILER) $(CFLAGS) ./client/client.cpp -o ./client/client

runserver: server
	./server/server

runserver2: server2
	./server/server2

runwsserver: wsserver
	./server/wsserver

runclient: client
	./client/client
