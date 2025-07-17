#include "server_process.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <atomic>
#include <iostream>
#include <sstream>
#include <string>
#include <system_error>
#include <thread>
#include <unordered_set>
#include <vector>

#include "common/http.h"
#include "common/queue.h"
#include "common/socket.h"

class ScopedThread {
    std::thread t;

   public:
    ScopedThread(std::thread t_) : t(std::move(t_)) {
        if (!t.joinable()) throw std::logic_error("not thread");
    }
    ~ScopedThread() { t.join(); };
    ScopedThread(ScopedThread const &) = delete;
    ScopedThread &operator=(ScopedThread const &) = delete;
};

std::string HttpServer::message_processing(std::string &&message) {
    std::unordered_set<std::string> words;
    std::vector<std::string> result;
    std::istringstream iss(message);
    std::ostringstream oss;
    std::string word;
    while (iss >> word) {
        if (words.insert(word).second) { result.push_back(word); }
    }
    for (int i = 0; i < result.size(); ++i) {
        if (i > 0) oss << " ";
        oss << result[i];
    }
    std::cout << oss.str() << "\n";
    return oss.str();
}

void HttpServer::setup_listener() {
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
void HttpServer::handler_client(Socket &&client) {
    while (connected) {
        std::string request = http.read_request(client.get_fd());
        HttpProtocol response(request);
        if (!response.is_valid()) {
            if (request.empty()) return;
            response.create(HttpStatus::BAD_REQUEST);
            http.send_request(client.get_fd(), response, "Invalid request");
            return;
        }
        response.create(HttpStatus::OK);
        http.send_request(client.get_fd(), response, "Request accepted");
        messages_queue.push(
            std::move(response.get_value_json(R"("message":")")));
    }
}
void HttpServer::reconnect_loop() {
    std::this_thread::sleep_for(std::chrono::seconds(2));
    while (running) {
        {
            std::unique_lock lock(reconnect_mutex);
            reconnect_condition.wait(lock,
                                     [this] { return !running || !connected; });

            if (!running) break;
        }

        for (int attempt = 1; attempt < 10 && !connected && running;
             ++attempt) {
            try {
                std::cout << "Wait to reconnect (" << attempt + 1 << ")...\n";
                server_display.connect(connect_config.ip, connect_config.port);
                connected = true;
                std::cout << "Reconnected success\n";
            } catch (const std::exception &e) {
                std::cerr << "Reconnect failed: " << e.what() << "\n";
                std::this_thread::sleep_for(std::chrono::seconds(attempt));
            }
        }

        if (!connected) {
            std::cerr << "Failed to reconnect\n";
            running = false;
        }
    }
}

void HttpServer::run() {
    try {
        ScopedThread reconnect_thread(
            std::thread([this] { reconnect_loop(); }));

        ScopedThread process_thread(std::thread([this] {
            try {
                while (running) {
                    try {
                        handler_client(listener.accept());
                    } catch (const std::exception &e) {
                        if (!running) break;
                        std::cerr << "Client handling error: " << e.what()
                                  << "\n";
                    }
                }
            } catch (...) {
                running = false;
                reconnect_condition.notify_one();
            }
        }));

        ScopedThread redirect_thread(
            std::thread([this] { redirect_message(); }));

        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

    } catch (const std::exception &e) {
        running = false;
        std::cerr << "Server fatal error: " << e.what() << "\n";
    }
}

void HttpServer::redirect_message() {
    while (running) {
        try {
            if (!connected) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                continue;
            }

            std::string message;
            messages_queue.wait_pop(message);
            std::string clean_message = message_processing(std::move(message));
            HttpProtocol request;
            request.create(HttpMethod::POST);
            request.add_header("Forwarded", server_config.host());

            if (!http.send_request(
                    server_display.get_fd(), request, clean_message)) {
                connected = false;
                reconnect_condition.notify_one();
            }

        } catch (const std::exception &e) {
            if (!running) break;
            std::cerr << "Redirect error: " << e.what() << "\n";
            connected = false;
            reconnect_condition.notify_one();
        }
    }
}
int main(int argc, char const *argv[]) {
    ConfigServer config;
    config.argumnet_parse(argc, argv);
    HttpServer server(config.server, config.connect_server);

    std::thread server_thread([&server] { server.run(); });

    server_thread.join();

    return 0;
}