#pragma once

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <string>
#include <system_error>

class Socket {
    int fd = -1;

   public:
    Socket() = default;
    explicit Socket(int fd) : fd(fd) {}
    Socket(int domain, int type, int protocol = 0) {
        fd = socket(domain, type, protocol);
        if (fd < 0) {
            throw std::system_error(
                errno, std::generic_category(), "socket creation failed");
        }
    }
    Socket& operator=(Socket&& other) noexcept {
        if (this != &other) {
            if (fd >= 0) close(fd);
            fd = other.fd;
            other.fd = -1;
        }
        return *this;
    }
    ~Socket() {
        if (fd >= 0) { close(fd); }
    }

    // Socket(const Socket&) = delete;

    Socket(Socket&& other) noexcept : fd(other.fd) { other.fd = -1; }

    // Socket& operator=(Socket&& other) noexcept;

    int get_fd() const;
    void bind(const sockaddr* addr, socklen_t addrlen);
    void listen(int backlog = SOMAXCONN);
    Socket accept(sockaddr* addr = nullptr, socklen_t* addrlen = nullptr);
    void connect(std::string_view ip, std::string_view port);
};