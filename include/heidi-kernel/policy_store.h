#pragma once

#include "heidi-kernel/resource_governor.h"

#include <string>

namespace heidi {

class PolicyStore {
public:
  explicit PolicyStore(std::string path);

  // Load policy from file, with validation and fallback to defaults on error
  GovernorPolicy load();

  // Save policy atomically
  void save(const GovernorPolicy& policy);

private:
  std::string path_;
};

} // namespace heidi