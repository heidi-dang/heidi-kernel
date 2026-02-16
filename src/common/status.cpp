#include "status.hpp"
#include <sstream>
#include <iomanip>

namespace heidi {

std::string Status::to_json() const {
    std::ostringstream oss;
    oss << std::boolalpha;
    oss << "{";
    oss << "\"ok\":" << (ok ? "true" : "false") << ",";
    oss << "\"pid\":" << pid << ",";
    oss << "\"uptime_ms\":" << uptime_ms << ",";
    oss << "\"version\":\"" << version << "\",";
    oss << "\"sock_path\":\"" << sock_path << "\",";
    oss << "\"build_git\":\"" << build_git << "\"";
    oss << "}";
    return oss.str();
}

Status Status::error(const std::string& msg) {
    Status s;
    s.ok = false;
    s.version = "\"error\":\"" + msg + "\"";
    return s;
}

} // namespace heidi
