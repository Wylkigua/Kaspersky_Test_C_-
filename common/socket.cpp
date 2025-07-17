#include "socket.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstring>
#include <iostream>
#include <map>
#include <system_error>
#include <unordered_map>
#include <utility>

#include "queue.h"

int Socket::get_fd() const {
    return fd;
}

void Socket::bind(const sockaddr* addr, socklen_t addrlen) {
    if (::bind(fd, addr, addrlen) < 0) {
        throw std::system_error(errno, std::generic_category(), "bind failed");
    }
}

void Socket::listen(int backlog) {
    if (::listen(fd, backlog) < 0) {
        throw std::system_error(
            errno, std::generic_category(), "listen failed");
    }
}

Socket Socket::accept(sockaddr* addr, socklen_t* addrlen) {
    int client_fd = ::accept(fd, addr, addrlen);
    if (client_fd < 0) {
        throw std::system_error(
            errno, std::generic_category(), "accept failed");
    }
    return Socket(client_fd);
}

void Socket::connect(std::string_view ip, std::string_view port) {
    addrinfo* ip_address_list = nullptr;
    addrinfo hints;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICSERV;
    hints.ai_protocol = 0;
    hints.ai_addr = NULL;
    hints.ai_addrlen = 0;
    hints.ai_canonname = NULL;
    hints.ai_next = NULL;
    int status = 0;
    if ((status = getaddrinfo(
             ip.data(), port.data(), &hints, &ip_address_list)) != 0) {
        throw std::system_error(
            errno, std::generic_category(), gai_strerror(status));
    }
    addrinfo* aip;
    for (aip = ip_address_list; aip != nullptr; aip = aip->ai_next) {
        if (!::connect(fd, aip->ai_addr, aip->ai_addrlen)) {
            break;
        } else {
            close(fd);
            fd = ::socket(AF_INET, SOCK_STREAM, 0);
        }
    }
    freeaddrinfo(ip_address_list);
    if (aip == nullptr) {
        throw std::system_error(
            errno, std::generic_category(), "connect failed");
    }
}