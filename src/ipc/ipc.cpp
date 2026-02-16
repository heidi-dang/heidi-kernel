#include "heidi-kernel/ipc.h"

#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace heidi {

std::string IpcProtocol::serialize(const IpcMessage& msg) {
  return msg.type + "\n";
}

IpcMessage IpcProtocol::deserialize(const std::string& data) {
  IpcMessage msg;
  std::istringstream iss(data);
  std::getline(iss, msg.type);
  return msg;
}

UnixSocketServer::UnixSocketServer(const std::string& path) : path_(path) {
  server_fd_ = socket(AF_UNIX, SOCK_STREAM, 0);
  if (server_fd_ < 0) {
    throw std::runtime_error("Failed to create socket");
  }

  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);

  unlink(path.c_str()); // Remove if exists
  if (bind(server_fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    throw std::runtime_error("Failed to bind socket");
  }

  if (listen(server_fd_, 5) < 0) {
    throw std::runtime_error("Failed to listen");
  }
}

UnixSocketServer::~UnixSocketServer() {
  stop();
}

void UnixSocketServer::serve_forever() {
  while (true) {
    int client_fd = accept(server_fd_, nullptr, nullptr);
    if (client_fd < 0)
      continue;
    handle_client(client_fd);
    close(client_fd);
  }
}

void UnixSocketServer::stop() {
  if (server_fd_ >= 0) {
    close(server_fd_);
    server_fd_ = -1;
  }
  unlink(path_.c_str());
}

void UnixSocketServer::handle_client(int client_fd) {
  char buffer[1024];
  ssize_t n = read(client_fd, buffer, sizeof(buffer) - 1);
  if (n <= 0)
    return;
  buffer[n] = '\0';

  IpcMessage request = IpcProtocol::deserialize(buffer);
  IpcMessage response;

  if (request_handler_) {
    std::string resp_str = request_handler_(request.type);
    write(client_fd, resp_str.c_str(), resp_str.size());
  } else {
    response.type = "error";
    std::string resp_str = IpcProtocol::serialize(response);
    write(client_fd, resp_str.c_str(), resp_str.size());
  }
}

} // namespace heidi