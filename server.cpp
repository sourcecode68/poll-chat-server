// server.cpp
//
// Simple poll-based group chat server (single process, no fork/threads).
//
// Build: g++ -std=c++17 -O2 -o server server.cpp
// Run:   ./server
//
// Protocol: the first thing a client sends after connecting is its display
// name, terminated by '\n'. After that, every line a client sends is
// broadcast to every other connected client, prefixed with "<name>: ".

#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>
#include <cstring>
#include <cerrno>
#include <csignal>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
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
            return false; // broken pipe / connection reset, etc.
        }
        sent += (size_t)n;
    }
    return true;
}

// Send `msg` to every connected client except `exclude_fd` (-1 excludes none).
static void broadcast(const vector<pollfd> &fds, int listen_fd, int exclude_fd,
                      const string &msg)
{
    for (const auto &pf : fds)
    {
        if (pf.fd == listen_fd || pf.fd == exclude_fd)
            continue;
        send_all(pf.fd, msg.c_str(), msg.size()); // ignore failures; recv will catch dead peers
    }
}

int main()
{
    // Don't die if we send() to a client that already closed its socket.
    signal(SIGPIPE, SIG_IGN);

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0)
    {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(listen_fd, IPPROTO_TCP, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        close(listen_fd);
        return 1;
    }

    if (listen(listen_fd, 16) < 0)
    {
        perror("listen");
        close(listen_fd);
        return 1;
    }

    cout << "Server listening on port " << PORT << endl;

    vector<pollfd> fds;
    fds.push_back({listen_fd, POLLIN, 0});

    unordered_map<int, string> names; // client fd -> display name

    for (;;)
    {
        int ready = poll(fds.data(), fds.size(), -1);
        if (ready < 0)
        {
            if (errno == EINTR)
                continue;
            perror("poll");
            break;
        }

        for (int i = 0; i < (int)fds.size(); i++)
        {
            if (!(fds[i].revents & POLLIN))
                continue;

            // New connection
            if (fds[i].fd == listen_fd)
            {
                int client_fd = accept(listen_fd, nullptr, nullptr);
                if (client_fd < 0)
                {
                    perror("accept");
                    continue;
                }

                // First message a client sends is its name, ending in '\n'.
                char namebuf[256] = {0};
                int n = recv(client_fd, namebuf, sizeof(namebuf) - 1, 0);
                if (n <= 0)
                {
                    close(client_fd);
                    continue;
                }
                string name(namebuf, n);
                while (!name.empty() && (name.back() == '\n' || name.back() == '\r'))
                    name.pop_back();
                if (name.empty())
                    name = "anon" + to_string(client_fd);

                names[client_fd] = name;
                fds.push_back({client_fd, POLLIN, 0});

                cout << "[+] " << name << " joined (fd " << client_fd << ")" << endl;
                broadcast(fds, listen_fd, client_fd,
                          "*** " + name + " joined the chat ***\n");
                continue;
            }

            // --- Message (or disconnect) from an existing client ---
            char buf[BUF_SIZE];
            int n = recv(fds[i].fd, buf, sizeof(buf) - 1, 0);

            if (n <= 0)
            { // 0 = clean disconnect, <0 = error
                string name = names.count(fds[i].fd) ? names[fds[i].fd] : "unknown";
                cout << "[-] " << name << " left (fd " << fds[i].fd << ")" << endl;

                close(fds[i].fd);
                names.erase(fds[i].fd);

                fds.erase(fds.begin() + i);
                i--; // re-check this index, now occupied by the next element

                broadcast(fds, listen_fd, -1, "*** " + name + " left the chat ***\n");
                continue;
            }

            buf[n] = '\0';
            string text(buf);
            if (text.empty() || text.back() != '\n')
                text += '\n';

            string out = names[fds[i].fd] + ": " + text;
            cout << out; // server-side log (already ends with '\n')

            broadcast(fds, listen_fd, fds[i].fd, out);
        }
    }

    close(listen_fd);
    return 0;
}