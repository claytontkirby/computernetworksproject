#!/bin/bash

#remove tracker files
rm -rf test_server/*.*

#run server
./server.out localhost 3456 10
