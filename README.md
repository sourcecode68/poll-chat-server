# Poll-Based Multi-Client Chat Server

A multi-client TCP chat application written in C++ using the Linux `poll()` system call for I/O multiplexing.

Unlike traditional process-per-client servers, this implementation uses a single process and an event-driven architecture to handle multiple simultaneous clients efficiently.

---

## Features

- Multiple concurrent clients
- TCP networking
- poll()-based I/O multiplexing
- Event-driven server architecture
- Real-time message broadcasting
- User nicknames
- Graceful client disconnect handling
- Single-process design

---

## Architecture
```text
                 +-------------------+
                 |  Listening Socket |
                 |     TCP :8080     |
                 +---------+---------+
                           |
                           v
                 +-------------------+
                 |    poll() Loop    |
                 | Monitors All FDs  |
                 +---------+---------+
                           |
        +------------------+------------------+
        |                  |                  |
        v                  v                  v

+---------------+  +---------------+  +---------------+
|   Client A    |  |   Client B    |  |   Client C    |
|   Socket FD4  |  |   Socket FD5  |  |   Socket FD6  |
+-------+-------+  +-------+-------+  +-------+-------+
        \                |                /
         \               |               /
          \              |              /
           +-------------+-------------+
                         |
                         v

                +-------------------+
                | Broadcast Engine  |
                | Message Routing   |
                +-------------------+

                         |
                         v

             Sends to all connected clients
```

---

## How It Works

The server:

1. Creates a listening TCP socket.
2. Registers the listening socket with `poll()`.
3. Waits for events.
4. Accepts new clients when connection requests arrive.
5. Reads messages from active clients.
6. Broadcasts messages to every connected client.

The server remains responsive without creating additional processes or threads.

---

## Technologies Used

- C++
- POSIX Sockets
- TCP/IP
- poll()
- Linux System Calls

---

## Building

Compile:

```bash
make
```

---

## Running

### Start Server

```bash
./server
```

Example:

```text
Server listening on port 8080
```

---

### Start Client

Open a new terminal:

```bash
./client
```

Start multiple clients to simulate a chat room.

---

## Example Session

Client 1:

```text
Alice: Hello everyone!
```

Client 2 receives:

```text
Alice: Hello everyone!
```

Client 3 receives:

```text
Alice: Hello everyone!
```

---

## Key Concepts Demonstrated

### Networking

- socket()
- bind()
- listen()
- accept()
- connect()
- recv()
- send()

### I/O Multiplexing

- poll()
- Event-driven programming
- Non-blocking server design

### Systems Programming

- File descriptors
- Event loops
- Resource management
- Client lifecycle management

---

## Why poll()?

A process-per-client design becomes expensive as the number of clients grows.

Using `poll()` allows a single server process to monitor many file descriptors simultaneously and react only when activity occurs.

Benefits:

- Lower memory usage
- Fewer context switches
- Better scalability
- Simpler deployment

---

## Future Improvements

- Private messaging
- Chat rooms
- User authentication
- Message history
- epoll() implementation
- TLS/SSL encryption
- Non-blocking sockets

---

## Concepts Learned

- Event-driven architecture
- Network server design
- Multiplexed I/O
- TCP communication
- Linux systems programming

---

## Author

Piyush Khanna
