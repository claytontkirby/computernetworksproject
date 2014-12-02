all: server.out client.out

server.out:
	g++ src/Tracker\ Server\ Skeleton.c -o test_server/server.out

client.out:
	g++ src/Peer\ Program\ Skeleton.c -o test_clients/client.out

clean:
	rm -rf test_server/server.out
	rm -rf test_clients/client.out
