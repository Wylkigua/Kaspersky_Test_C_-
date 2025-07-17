

Сборка проекта
  mkdir build && cd build
  cmake .. && make

Usage
  ./server_display [server_ip] [server_port] [connect_ip] [connect_port]

Argument
  server_ip: IP address to bind (default: 127.0.0.1)

  server_port: Port to listen on (default: 8176)

  connect_ip: Allowed forwarded IP (default: 127.0.0.1)

  connect_port: Allowed forwarded port (default: 9867)
