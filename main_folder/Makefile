all: server client

server:
	g++ src/Tracker\ Server\ Skeleton.c -o server.out -w -pthread

client:
	g++ src/Peer\ Program\ Skeleton.c -o client.out -w -pthread

clean:
	rm -rf server.out
	rm -rf client.out
