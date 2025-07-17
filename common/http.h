#pragma once

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <chrono>
#include <cstring>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <system_error>
#include <unordered_map>
#include <utility>

struct Config {
    std::string_view ip;
    std::string_view port;
    std::string host();
};
struct ConfigServer {
    void argumnet_parse(int argc, char const * argv[]);
    Config client, server, connect_server;
};

enum class HttpStatus { OK = 200, BAD_REQUEST = 400 };

enum class HttpMethod { POST };

class HttpProtocol {
   public:
    HttpProtocol() = default;
    HttpProtocol(std::string& message_protocol)
        : message_protocol(message_protocol) {}
    void create(HttpMethod method);
    void create(HttpStatus status);
    void add_header(std::string name, std::string value);
    std::string get_header(std::string_view header);
    std::string get_value_json(const std::string& key);
    void add_body(std::string_view data_body);
    std::string get_body();
    std::string to_string();
    bool is_valid();

   private:
    std::string get_status(HttpStatus status) const;
    std::string get_method(HttpMethod method) const;

    std::ostringstream oss;
    std::map<std::string, std::string> headers;
    std::string_view method;
    std::string_view starting_line;
    std::string_view body;
    std::string message_protocol;
    const std::string_view protocol = "HTTP/1.1";
};

class Http {
   public:
    int send_request(int socket,
                     HttpProtocol& protocol,
                     const std::string& message);
    std::string read_request(int socket);

   private:
    std::string create_json_body(const std::string& message);
    int send_message(int connect, const std::string& message);
    int read_message(int connect, std::string& message);
};

sockaddr_in convert_to_network(std::string_view ip, std::string_view port);
