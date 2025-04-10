# Socket Programming Chat Application

A multi-threaded chat server with private messaging and authentication, deployable on Render.com.

## Features
- Multi-threaded server handling multiple clients
- Group chat and private messaging
- SQLite database for user accounts and message history
- Password hashing with bcrypt
- Command-line client interface

## Deployment on Render.com

1. **Create a Render account** at [render.com](https://render.com/)
2. **Create a new Web Service** and connect your GitHub repository
3. Configure with these settings:
   - **Build Command**: `make`
   - **Start Command**: `./start.sh`
   - **Environment Variables**:
     - `PORT`: `10000`
     - `DB_PATH`: `/opt/render/project/server/chat.db`
4. Deploy!

## Local Development

### Server
```bash
cd server
make
./server
