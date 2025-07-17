#include "http.h"

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
#include <system_error>
#include <unordered_map>
#include <utility>

std::string Config::host() {
    std::string host;
    host += ip;
    host += ":";
    host += port;
    return host;
}

void ConfigServer::argumnet_parse(int argc, char const * argv[]) {
    server.ip = "127.0.0.1";
    server.port = "8176";
    connect_server.ip = "127.0.0.1";
    connect_server.port = "9867";
    switch (argc) {
        case 3:
            server.ip = argv[1];
            server.port = argv[2];
            break;
        case 5:
            server.ip = argv[1];
            server.port = argv[2];
            connect_server.ip = argv[3];
            connect_server.port = argv[4];
            break;
        default: throw std::invalid_argument("invalid argument");
    }
}

void HttpProtocol::create(HttpMethod method) {
    oss << get_method(method) << " " << protocol << "\r\n";
}
void HttpProtocol::create(HttpStatus status) {
    oss << protocol << " ";
    oss << static_cast<int>(status) << " ";
    oss << get_status(status) << "\r\n";
}
void HttpProtocol::add_header(std::string name, std::string value) {
    headers[std::string(name)] = std::string(value);
}
std::string HttpProtocol::get_header(std::string_view header) {
    size_t start_pos = message_protocol.find(header);
    if (start_pos == std::string::npos) return "";
    size_t end_pos = message_protocol.find(
        "\r\n", std::exchange(start_pos, start_pos + header.size() + 2));
    if (end_pos == std::string::npos) return "";
    std::string head = message_protocol.substr(start_pos, end_pos - start_pos);
    return head;
}
void HttpProtocol::add_body(std::string_view data_body) {
    body = data_body;
}
std::string HttpProtocol::get_value_json(const std::string& key) {
    // std::string_view target = R"("message":")";
    size_t start_pos = message_protocol.find(key);
    start_pos += key.size();
    return message_protocol.substr(start_pos,
                                   message_protocol.size() - start_pos - 2);
}

std::string HttpProtocol::get_body() {
    size_t start_pos = message_protocol.find("\r\n\r\n");
    start_pos += 3;
    return message_protocol.substr(start_pos,
                                   message_protocol.size() - start_pos);
}
std::string HttpProtocol::to_string() {
    for (const auto& [name, value] : headers)
        oss << name << ": " << value << "\r\n";
    oss << "\r\n";
    if (!body.empty()) oss << body;
    return oss.str();
}
bool HttpProtocol::is_valid() {
    std::string content_lenght = get_header("Content-Length");
    if (content_lenght.empty()) return false;
    int length = std::stoi(content_lenght);
    return (length > 0 && !get_header("Forwarded").empty() &&
            get_header("Content-Type") == "application/json");
}

std::string HttpProtocol::get_status(HttpStatus status) const {
    static const std::unordered_map<HttpStatus, std::string> messages = {
        {HttpStatus::OK, "OK"},
        {HttpStatus::BAD_REQUEST, "Bad Request"},
    };

    auto it = messages.find(status);
    return it != messages.end() ? it->second : "Unknown Status";
}
std::string HttpProtocol::get_method(HttpMethod method) const {
    static const std::unordered_map<HttpMethod, std::string> methods = {
        {HttpMethod::POST, "POST"},
    };

    auto it = methods.find(method);
    return it != methods.end() ? it->second : "Unknown Method";
}

int Http::send_request(int socket,
                       HttpProtocol& protocol,
                       const std::string& message) {
    std::string body = create_json_body(message);
    protocol.add_header("Content-Type", "application/json");
    protocol.add_header("Content-Length", std::to_string(body.size()));
    protocol.add_body(std::move(body));
    return send_message(socket, protocol.to_string().data());
}
std::string Http::read_request(int socket) {
    std::string request_message;
    std::string buffer;
    size_t buffer_length = 150;
    buffer.resize(buffer_length);
    size_t nread = read_message(socket, buffer);
    request_message.append(buffer, 0, nread);
    if (nread == buffer.size()) {
        std::string_view requires_header = "Content-Length: ";
        size_t start_pos = buffer.find(requires_header);
        if (start_pos == std::string::npos) { return ""; }
        start_pos += requires_header.size();
        size_t end_pos = buffer.find('\n', start_pos);
        std::string size_data = buffer.substr(start_pos, end_pos - start_pos);
        int total_size_data = std::stoi(size_data);
        end_pos = buffer.find("\r\n\r\n");
        total_size_data -= (nread - end_pos - 3);

        while (total_size_data > 0) {
            nread = read_message(socket, buffer);
            total_size_data -= nread;
            request_message.append(buffer, 0, nread);
        }
    }
    return request_message;
}
std::string Http::create_json_body(const std::string& message) {
    return "{\"message\":\"" + message + "\"}";
}
int Http::send_message(int connect, const std::string& message) {
    const char* buffer = message.data();
    size_t length = message.size();
    ssize_t received = 0;
    ssize_t total_write = 0;
    while (length > 0) {
        received = send(connect, buffer + total_write, length, MSG_NOSIGNAL);
        if (received < 0) {
            if (errno == EPIPE) return false;
            throw std::system_error(
                errno, std::generic_category(), "Error send message server");
        }
        total_write += received;
        length -= received;
    }
    return (total_write == static_cast<ssize_t>(message.size()));
}
int Http::read_message(int connect, std::string& message) {
    ssize_t received = -1;
    if ((received = recv(connect, message.data(), message.size(), 0)) < 0) {
        throw std::system_error(
            errno, std::generic_category(), "failed read message");
    }
    return received;
}

sockaddr_in convert_to_network(std::string_view ip, std::string_view port) {
    sockaddr_in address;
    if (inet_pton(AF_INET, ip.data(), &address.sin_addr) < 0) {
        throw std::system_error(
            errno, std::generic_category(), "Invalid IP address");
    }
    uint16_t int_port = static_cast<uint16_t>(std::stoi(port.data()));
    if (!int_port) { throw std::logic_error("Invalid port"); }
    address.sin_port = htons(int_port);
    address.sin_family = AF_INET;
    return address;
}