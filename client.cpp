#include "client.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <iostream>
#include <system_error>

#include "common/http.h"
#include "common/socket.h"

void Client::connect() {
    try {
        Socket connection(AF_INET, SOCK_STREAM);
        connection.connect(connect_config.ip, connect_config.port);
        handler_message(std::move(connection));
    } catch (const std::system_error &e) {
        std::cerr << "System error: " << e.what() << '\n'
                  << "Error details: " << e.code().message() << '\n';
        return;
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << '\n';
        return;
    } catch (...) {
        std::cerr << "Unknown error occurred\n";
        return;
    }
}
void Client::handler_message(Socket &&connection) {
    while (true) {
        std::string message;
        if (!input(message)) return;

        Http http;
        HttpProtocol request;
        request.create(HttpMethod::POST);
        request.add_header("Forwarded", get_local_address(connection.get_fd()));
        http.send_request(connection.get_fd(), request, message);
        std::string answer = http.read_request(connection.get_fd());

        if (answer.empty()) throw std::runtime_error("Server is not available");
    }
}
bool Client::input(std::string &message) {
    if (!std::getline(std::cin, message)) {
        if (std::cin.eof()) {
            std::cout << "Connection close\n";
            return false;
        }
        throw std::runtime_error("Invalid input");
    }
    return true;
}
std::string Client::get_local_address(int socket) const {
    sockaddr_in address {};
    socklen_t len = sizeof(address);

    if (getsockname(socket, (sockaddr *) &address, &len) < 0) {
        throw std::runtime_error("Getsockname failed");
    }
    char buffer[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &address.sin_addr, buffer, sizeof(buffer));
    uint16_t port = static_cast<uint16_t>(htons(address.sin_port));
    std::ostringstream oss;
    oss << buffer << ":" << std::to_string(port);
    return oss.str();
}

int main(int argc, char const *argv[]) {
    ConfigServer config;
    config.argumnet_parse(argc, argv);
    Client client(config.server);
    client.connect();
    return 0;
}
