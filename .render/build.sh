#!/bin/bash
# Install libsodium first
sudo apt-get update
sudo apt-get install -y libsodium-dev

# Then compile
cd server
gcc -Wall -Wextra server.c database.c auth.c -o server -lsqlite3 -lpthread -lsodium
chmod +x server