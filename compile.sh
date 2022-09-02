#!/bin/bash

rm -f main

g++-10 main.cpp -o main -Wall `pkg-config --cflags opencv4` `pkg-config --libs opencv4` -O3 -finline-functions -DNDEBUG -pthread -std=c++17