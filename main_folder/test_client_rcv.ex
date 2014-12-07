#!/bin/bash

#clear screen
clear

#remove tracker and image files
rm -f test_clients/client_6/*.*
rm -f test_clients/client_7/*.*
rm -f test_clients/client_8/*.*
rm -f test_clients/client_9/*.*
rm -f test_clients/client_10/*.*

#run receiving client
./client.out rcv 6 &

./client.out rcv 7 &

./client.out rcv 8 &

./client.out rcv 9 &

./client.out rcv 10 & wait
