// client.cpp
//
// Chat client for server.cpp.
//
// Build: g++ -std=c++17 -O2 -o client client.cpp
// Run:   ./client <server-ipv4>
//
// Prompts for a display name, sends it to the server, then polls between
// stdin (your typed messages) and the socket (messages from other clients).

#include <iostream>
#include <string>
#include <cstring>
#include <cerrno>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;

#define PORT 9999
#define BUF_SIZE 512

// Loop on send() until all `len` bytes are written, or the connection breaks.
static bool send_all(int fd, const char *data, size_t len)
{
    size_t sent = 0;
    while (sent < len)
    {
        ssize_t n = send(fd, data + sent, len - sent, 0);
        if (n < 0)
        {
            if (errno == EINTR)
                continue;
            return false;
        }
        sent += (size_t)n;
    }
    return true;
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        cerr << "Usage: " << argv[0] << " <server-ipv4>" << endl;
        return 1;
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("socket");
        return 1;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, argv[1], &addr.sin_addr) <= 0)
    {
        cerr << "Invalid IPv4 address: " << argv[1] << endl;
        close(sockfd);
        return 1;
    }

    if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("connect");
        close(sockfd);
        return 1;
    }

    cout << "Connected. Enter your name: " << flush;

    char namebuf[256] = {0};
    if (!fgets(namebuf, sizeof(namebuf), stdin))
    {
        cerr << "No name entered, exiting." << endl;
        close(sockfd);
        return 1;
    }
    if (!send_all(sockfd, namebuf, strlen(namebuf)))
    {
        perror("send (name)");
        close(sockfd);
        return 1;
    }

    cout << "Joined chat. Type a message and press Enter. Ctrl+D to quit." << endl;

    struct pollfd fds[2] = {
        {STDIN_FILENO, POLLIN, 0},
        {sockfd, POLLIN, 0}};

    for (;;)
    {
        int ready = poll(fds, 2, -1);
        if (ready < 0)
        {
            if (errno == EINTR)
                continue;
            perror("poll");
            break;
        }

        // --- User typed something ---
        if (fds[0].revents & POLLIN)
        {
            char buf[BUF_SIZE] = {0};
            if (!fgets(buf, sizeof(buf), stdin))
            {
                cout << "Disconnecting..." << endl;
                break;
            }
            if (!send_all(sockfd, buf, strlen(buf)))
            {
                perror("send");
                break;
            }
        }

        // --- Server sent something ---
        if (fds[1].revents & POLLIN)
        {
            char buf[BUF_SIZE] = {0};
            int n = recv(sockfd, buf, sizeof(buf) - 1, 0);
            if (n == 0)
            {
                cout << "Server closed the connection." << endl;
                break;
            }
            else if (n < 0)
            {
                if (errno == EINTR)
                    continue;
                perror("recv");
                break;
            }
            buf[n] = '\0';
            cout << buf << flush;
        }

        if (fds[1].revents & (POLLHUP | POLLERR))
        {
            cout << "Connection lost." << endl;
            break;
        }
    }

    close(sockfd);
    return 0;
}