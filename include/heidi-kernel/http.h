#pragma once

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
  void serve_forever();

public:
  static HttpRequest parse_request(const std::string& data);

private:
  void handle_client(int client_fd);
  std::string format_response(const HttpResponse& resp) const;

  std::string address_;
  uint16_t port_;
  int server_fd_ = -1;
  std::vector<std::pair<std::string, RequestHandler>> handlers_;
};

} // namespace heidi
