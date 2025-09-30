#!/usr/bin/env bash

g++ -o main -Og -Wall main.cpp ./glad/src/gl.c -I ./glad/include -l glfw -I . "$@"
