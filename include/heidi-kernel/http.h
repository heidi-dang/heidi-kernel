#pragma once

#include <csignal>
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace heidi {

struct HttpRequest {
  std::string_view method;
  std::string_view path;
  std::string_view body;
  std::map<std::string, std::string> headers;
};

struct HttpResponse {
  uint16_t status_code = 200;
  std::string_view status_text = "OK";
  std::string body;
  std::map<std::string, std::string> headers;
};

using RequestHandler = std::function<HttpResponse(const HttpRequest&)>;

class HttpServer {
public:
  explicit HttpServer(std::string_view address, uint16_t port);
  ~HttpServer();

  void register_handler(std::string_view path, RequestHandler handler);
  void serve_forever(volatile std::sig_atomic_t* shutdown_flag = nullptr);

private:
  void handle_client(int client_fd);
  HttpRequest parse_request(const std::string& data) const;
  std::string format_response(const HttpResponse& resp) const;

  std::string address_;
  uint16_t port_;
  int server_fd_ = -1;
  std::vector<std::pair<std::string, RequestHandler>> handlers_;
};

} // namespace heidi
