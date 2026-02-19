#pragma once

#include <string>
#include <chrono>
#include <sstream>
#include <iomanip>

namespace heidi {

inline std::string json_escape(std::string_view s) {
    std::ostringstream oss;
    oss << '"';
    for (char c : s) {
        switch (c) {
            case '"': oss << "\\\""; break;
            case '\\': oss << "\\\\"; break;
            case '\b': oss << "\\b"; break;
            case '\f': oss << "\\f"; break;
            case '\n': oss << "\\n"; break;
            case '\r': oss << "\\r"; break;
            case '\t': oss << "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    oss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
                } else {
                    oss << c;
                }
        }
    }
    oss << '"';
    return oss.str();
}

struct Event {
    std::string id;
    std::chrono::system_clock::time_point timestamp;
    std::string type;
    std::string payload;

    std::string to_json() const {
        auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(
            timestamp.time_since_epoch()).count();
        return "{\"id\":" + json_escape(id) +
               ",\"timestamp\":" + std::to_string(ts) +
               ",\"type\":" + json_escape(type) +
               ",\"payload\":" + payload + "}";
    }
};

} // namespace heidi
