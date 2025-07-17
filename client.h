#pragma once
#include "common/http.h"
#include "common/socket.h"

class Client {
    Config connect_config;

   public:
    Client(Config connect_config) : connect_config(connect_config) {}
    void connect();

   private:
    void handler_message(Socket &&connection);
    bool input(std::string &message);
    std::string get_local_address(int socket) const;
};