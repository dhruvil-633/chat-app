#!/bin/bash
cd server
gcc -Wall -Wextra server.c database.c auth.c -o server -lsqlite3 -lpthread
chmod +x server