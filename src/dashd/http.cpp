#include "heidi-kernel/http.h"

#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <string>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>

namespace {

std::string_view trim(std::string_view sv) {
  while (!sv.empty() && (sv[0] == ' ' || sv[0] == '\t')) {
    sv.remove_prefix(1);
  }
  while (!sv.empty() &&
         (sv.back() == ' ' || sv.back() == '\t' || sv.back() == '\n' || sv.back() == '\r')) {
    sv.remove_suffix(1);
  }
  return sv;
}

} // namespace

namespace heidi {

HttpServer::HttpServer(std::string_view address, uint16_t port) : address_(address), port_(port) {}

HttpServer::~HttpServer() {
  if (server_fd_ >= 0) {
    close(server_fd_);
  }
}

void HttpServer::register_handler(std::string_view path, RequestHandler handler) {
  handlers_.emplace_back(std::string(path), std::move(handler));
}

void HttpServer::serve_forever() {
  server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd_ < 0) {
    return;
  }

  int opt = 1;
  setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port_);
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

  if (bind(server_fd_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
    close(server_fd_);
    server_fd_ = -1;
    return;
  }

  listen(server_fd_, 10);

  while (true) {
    int client_fd = accept(server_fd_, nullptr, nullptr);
    if (client_fd < 0) {
      break;
    }
    handle_client(client_fd);
    close(client_fd);
  }
}

void HttpServer::handle_client(int client_fd) {
  std::string request_buffer;
  char temp_buf[4096];
  ssize_t n;
  size_t body_start = std::string::npos;
  size_t content_length = 0;
  bool headers_parsed = false;
  const size_t MAX_REQUEST_SIZE = 1024 * 1024; // 1 MB

  while (true) {
    if (request_buffer.size() >= MAX_REQUEST_SIZE) {
      const char* resp_413 = "HTTP/1.1 413 Payload Too Large\r\nContent-Length: 0\r\n\r\n";
      write(client_fd, resp_413, strlen(resp_413));
      return;
    }

    n = read(client_fd, temp_buf, sizeof(temp_buf));
    if (n <= 0) {
      break;
    }
    request_buffer.append(temp_buf, n);

    if (!headers_parsed) {
      body_start = request_buffer.find("\r\n\r\n");
      if (body_start != std::string::npos) {
        body_start += 4; // Skip \r\n\r\n
        headers_parsed = true;

        // Find Content-Length
        size_t pos = 0;
        while (pos < body_start) {
          size_t line_end = request_buffer.find("\r\n", pos);
          if (line_end == std::string::npos || line_end >= body_start)
            break;

          std::string_view line(request_buffer.c_str() + pos, line_end - pos);
          if (line.size() >= 15 && strncasecmp(line.data(), "Content-Length:", 15) == 0) {
            size_t val_start = 15;
            while (val_start < line.size() && line[val_start] == ' ')
              val_start++;
            if (val_start < line.size()) {
              try {
                content_length = std::stoul(std::string(line.substr(val_start)));
              } catch (...) {
                content_length = 0;
              }
            }
            break;
          }
          pos = line_end + 2;
        }
      }
    }

    if (headers_parsed) {
      if (request_buffer.size() >= body_start + content_length) {
        break;
      }
    } else if (n < (ssize_t)sizeof(temp_buf)) {
      // If we read less than buffer size and headers not found, maybe end of stream?
      // But read returns 0 on EOF. n < sizeof(temp_buf) just means data is available.
      // We should keep reading until headers found or EOF.
    }
  }

  if (request_buffer.empty()) {
    return;
  }

  HttpRequest req = parse_request(request_buffer);

  HttpResponse resp;
  resp.headers["Content-Type"] = "application/json";
  resp.headers["Access-Control-Allow-Origin"] = "*";

  for (const auto& [path, handler] : handlers_) {
    if (req.path == path) {
      resp = handler(req);
      break;
    }
  }

  if (resp.body.empty()) {
    resp.status_code = 404;
    resp.status_text = "Not Found";
    resp.body = "{\"error\":\"not found\"}";
  }

  std::string response = format_response(resp);
  write(client_fd, response.c_str(), response.size());
}

HttpRequest HttpServer::parse_request(const std::string& data) const {
  HttpRequest req;

  size_t pos = data.find("\r\n\r\n");
  std::string_view body_view;
  if (pos != std::string::npos) {
    body_view = std::string_view(data).substr(pos + 4);
  }

  size_t line_end = data.find("\r\n");
  if (line_end != std::string::npos) {
    std::string_view request_line(data.c_str(), line_end);
    size_t sp1 = request_line.find(' ');
    size_t sp2 = request_line.find(' ', sp1 + 1);
    if (sp1 != std::string::npos && sp2 != std::string::npos) {
      req.method = trim(request_line.substr(0, sp1));
      req.path = trim(request_line.substr(sp1 + 1, sp2 - sp1 - 1));
    }
  }

  req.body = body_view;
  return req;
}

std::string HttpServer::format_response(const HttpResponse& resp) const {
  std::string resp_str = "HTTP/1.1 ";
  resp_str += std::to_string(resp.status_code) + " " + std::string(resp.status_text) + "\r\n";

  for (const auto& [key, value] : resp.headers) {
    resp_str += key + ": " + value + "\r\n";
  }

  resp_str += "Content-Length: " + std::to_string(resp.body.size()) + "\r\n";
  resp_str += "\r\n";
  resp_str += resp.body;

  return resp_str;
}

} // namespace heidi
