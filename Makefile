COMPILER = clang++

server:
	$(COMPILER) ./server/server.cpp -o ./server/server

client:
	$(COMPILER) ./client/client.cpp -o ./client/client

runserver: client
	./server/server

runclient: server
	./client/client
