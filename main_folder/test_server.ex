#!/bin/bash

#remove tracker files
rm -rf test_server/*.track

#run server
./server.out localhost 3456 10
