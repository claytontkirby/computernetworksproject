all: server client

server:
	g++ src/Tracker\ Server\ Skeleton.c -o server.out -w -pthread

client:
	g++ src/Peer\ Program\ Skeleton.c -o client.out -w -pthread

clean:
	rm -rf server.out
	rm -rf client.out
	rm -f test_clients/client_6/*.track 
	rm -f test_clients/client_6/*.jpg
	rm -f test_clients/client_7/*.track 
	rm -f test_clients/client_7/*.jpg
	rm -f test_clients/client_8/*.track 
	rm -f test_clients/client_8/*.jpg
	rm -f test_clients/client_9/*.track 
	rm -f test_clients/client_9/*.jpg
	rm -f test_clients/client_10/*.track 
	rm -f test_clients/client_10/*.jpg

