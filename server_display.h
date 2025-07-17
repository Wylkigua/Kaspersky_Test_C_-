#pragma once

#include "common/http.h"
#include "common/socket.h"

class HttpServerDisplay {
    Socket listener;
    Config server_config;
    Config connect_config;
    sockaddr_in address = {};
    sockaddr_in connect_address = {};
    Http http;

   public:
    HttpServerDisplay(Config server_config, Config target_config)
        : server_config(std::move(server_config))
        , connect_config(std::move(target_config)) {
        setup_listener();
    }
    void run();

   private:
    void handler_client(Socket &&client);
    void setup_listener();
};
