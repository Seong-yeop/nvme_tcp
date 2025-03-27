#!/bin/bash

make clean
make
./nvme_tcp 2>&1 | tee out.txt
sed -i 's/\x1b\[[0-9;]*m//g' out.txt
