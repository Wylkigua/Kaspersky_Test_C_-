#include "server_display.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <iostream>
#include <string>
#include <system_error>

#include "common/http.h"
#include "common/socket.h"

void HttpServerDisplay::run() {
    try {
        while (true) { handler_client(listener.accept()); }
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

void HttpServerDisplay::handler_client(Socket &&client) {
    while (true) {
        std::string message = http.read_request(client.get_fd());
        HttpProtocol request(message);
        if (message.empty() || !request.is_valid() ||
            request.get_header("Forwarded") != connect_config.host()) {
            return;
        }
        std::cout << request.get_value_json(R"("message":")") << "\n";
    }
}
void HttpServerDisplay::setup_listener() {
    address = convert_to_network(server_config.ip, server_config.port);
    listener = Socket(AF_INET, SOCK_STREAM);
    int reuse = 1;
    if (setsockopt(listener.get_fd(),
                   SOL_SOCKET,
                   SO_REUSEADDR,
                   &reuse,
                   sizeof(reuse))) {
        throw std::system_error(
            errno, std::generic_category(), "error reuseaddres");
    }
    listener.bind(reinterpret_cast<const sockaddr *>(&address),
                  sizeof(address));
    listener.listen();
}

int main(int argc, char const *argv[]) {
    ConfigServer config;
    config.argumnet_parse(argc, argv);
    HttpServerDisplay server(config.server, config.connect_server);
    server.run();
    return 0;
}
