#!/bin/bash

install -d /usr/local/include/monkeyKing
cp src/include/* /usr/local/include/monkeyKing -r
install -t /usr/local/lib/ out/lib/libmonkeyKing.so 
