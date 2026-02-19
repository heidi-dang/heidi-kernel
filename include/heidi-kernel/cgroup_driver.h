#pragma once

#include "heidi-kernel/gov_rule.h"

#include <cstdint>
#include <optional>
#include <string>

namespace heidi {
namespace gov {

class CgroupDriver {
public:
  enum class Capability : uint8_t {
    NONE = 0,
    CPU = 1 << 0,
    MEMORY = 1 << 1,
    PIDS = 1 << 2,
  };

  CgroupDriver();
  ~CgroupDriver();

  bool is_available() const {
    return available_;
  }
  Capability capability() const {
    return capability_;
  }

  void set_enabled(bool enabled) {
    enabled_ = enabled;
  }
  bool is_enabled() const {
    return enabled_;
  }

  struct ApplyResult {
    bool success = false;
    int err = 0;
    std::string error_detail;
    Capability applied = Capability::NONE;
  };

  ApplyResult apply(int32_t pid, const CpuPolicy& cpu, const MemPolicy& mem,
                    const PidsPolicy& pids);

  void cleanup(int32_t pid);

private:
  bool detect();
  bool create_base_dir();

  bool available_ = false;
  bool enabled_ = false;
  Capability capability_ = Capability::NONE;
  std::string base_path_;
};

constexpr CgroupDriver::Capability operator|(CgroupDriver::Capability a,
                                             CgroupDriver::Capability b) {
  return static_cast<CgroupDriver::Capability>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
constexpr bool has_capability(CgroupDriver::Capability caps, CgroupDriver::Capability cap) {
  return (static_cast<uint8_t>(caps) & static_cast<uint8_t>(cap)) != 0;
}

} // namespace gov
} // namespace heidi
