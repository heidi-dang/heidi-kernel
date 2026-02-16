#include <string>

#include "heidi-kernel/http.h"

#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

namespace {

std::string_view trim(std::string_view sv) {
    while (!sv.empty() && (sv[0] == ' ' || sv[0] == '\t')) {
        sv.remove_prefix(1);
    }
    while (!sv.empty() && (sv.back() == ' ' || sv.back() == '\t' || sv.back() == '\n' || sv.back() == '\r')) {
        sv.remove_suffix(1);
    }
    return sv;
}

} // namespace

namespace heidi {

HttpServer::HttpServer(std::string_view address, uint16_t port)
    : address_(address), port_(port) {}

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
    char buf[4096];
    ssize_t n = read(client_fd, buf, sizeof(buf) - 1);
    if (n <= 0) {
        return;
    }
    buf[n] = '\0';

    HttpRequest req = parse_request(std::string(buf, n));

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
    std::string status_code_str = std::to_string(resp.status_code);
    std::string content_length_str = std::to_string(resp.body.size());

    size_t estimated_size = 32; // "HTTP/1.1 " + " " + "\r\n" + "Content-Length: " + "\r\n\r\n" approx
    estimated_size += status_code_str.size();
    estimated_size += resp.status_text.size();
    for (const auto& [key, value] : resp.headers) {
        estimated_size += key.size() + value.size() + 4;
    }
    estimated_size += content_length_str.size();
    estimated_size += resp.body.size();

    std::string resp_str;
    resp_str.reserve(estimated_size);

    resp_str += "HTTP/1.1 ";
    resp_str += status_code_str;
    resp_str += " ";
    resp_str += resp.status_text;
    resp_str += "\r\n";

    for (const auto& [key, value] : resp.headers) {
        resp_str += key;
        resp_str += ": ";
        resp_str += value;
        resp_str += "\r\n";
    }

    resp_str += "Content-Length: ";
    resp_str += content_length_str;
    resp_str += "\r\n\r\n";
    resp_str += resp.body;

    return resp_str;
}

}
