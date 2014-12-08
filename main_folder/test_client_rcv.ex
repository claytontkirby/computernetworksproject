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

MD5_CLIENT1=$(md5 -q "test_clients/client_1/picture-wallpaper.jpg")
MD5_CLIENT6=$(md5 -q "test_clients/client_6/picture-wallpaper.jpg")
MD5_CLIENT7=$(md5 -q "test_clients/client_7/picture-wallpaper.jpg")
MD5_CLIENT8=$(md5 -q "test_clients/client_8/picture-wallpaper.jpg")
MD5_CLIENT9=$(md5 -q "test_clients/client_9/picture-wallpaper.jpg")
MD5_CLIENT10=$(md5 -q "test_clients/client_10/picture-wallpaper.jpg")

if [ "$MD5_CLIENT1" = "$MD5_CLIENT6" ]; then
	echo "I am client_6, and I received the file correctly!"
else
	echo "Client_6 did not receive the file correctly."
fi

if [ "$MD5_CLIENT1" = "$MD5_CLIENT7" ]; then
	echo "I am client_7, and I received the file correctly!"
else
	echo "Client_7 did not receive the file correctly."
fi

if [ "$MD5_CLIENT1" = "$MD5_CLIENT8" ]; then
	echo "I am client_8, and I received the file correctly!"
else
	echo "Client_8 did not receive the file correctly."
fi

if [ "$MD5_CLIENT1" = "$MD5_CLIENT9" ]; then
	echo "I am client_9, and I received the file correctly!"
else
	echo "Client_9 did not receive the file correctly."
fi

if [ "$MD5_CLIENT1" = "$MD5_CLIENT10" ]; then
	echo "I am client_10, and I received the file correctly!"
else
	echo "Client_10 did not receive the file correctly."
fi