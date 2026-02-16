#pragma once

#include <string>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace heidi {

struct IpcMessage {
    std::string type;
    nlohmann::json payload;
};

class IpcProtocol {
public:
    static std::string serialize(const IpcMessage& msg);
    static IpcMessage deserialize(const std::string& data);
};

class UnixSocketServer {
public:
    explicit UnixSocketServer(const std::string& path);
    ~UnixSocketServer();

    void serve_forever();
    void stop();

private:
    void handle_client(int client_fd);
    std::string path_;
    int server_fd_ = -1;
    bool running_ = false;
};

} // namespace heidi