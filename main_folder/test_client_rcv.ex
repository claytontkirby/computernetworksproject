#!/bin/bash

#clear screen
clear

#remove tracker and image files
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

#run receiving client
./client.out rcv 6 &

./client.out rcv 7 &

./client.out rcv 8 &

./client.out rcv 9 &

./client.out rcv 10 & wait
