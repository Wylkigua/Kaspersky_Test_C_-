#pragma once

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstring>
#include <iostream>
#include <map>
#include <system_error>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "common/http.h"
#include "common/queue.h"
#include "common/socket.h"

class HttpServer {
    Socket listener;
    Socket server_display;
    Config server_config;
    Config connect_config;
    threadsafe_queue messages_queue;
    std::mutex reconnect_mutex;
    std::condition_variable reconnect_condition;
    sockaddr_in address = {};
    Http http;
    std::atomic<bool> connected {false};
    std::atomic<bool> running {true};

    std::string message_processing(std::string &&message);

    void setup_listener();
    void handler_client(Socket &&client);
    void reconnect_loop();

   public:
    HttpServer(Config server_config, Config target_config)
        : server_config(std::move(server_config))
        , connect_config(std::move(target_config)) {
        setup_listener();
    }
    void run();
    void redirect_message();
};