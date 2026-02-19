#include "heidi-kernel/cgroup_driver.h"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sstream>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/types.h>
#include <unistd.h>

namespace heidi {
namespace gov {

namespace {

constexpr const char* kDefaultCgroupPath = "/sys/fs/cgroup/heidi";
constexpr uint64_t kCpuPeriod = 100000ULL;

bool is_cgroup2() {
  struct statfs fs;
  if (statfs("/sys/fs/cgroup", &fs) != 0) {
    return false;
  }
  return fs.f_type == 0x63677270;
}

bool check_controller(const std::string& controllers_path, const char* ctrl) {
  std::string path = controllers_path + "/" + ctrl;
  return access(path.c_str(), R_OK) == 0;
}

} // namespace

CgroupDriver::CgroupDriver() {
  if (!enabled_) {
    available_ = false;
    return;
  }

  available_ = detect();
}

CgroupDriver::~CgroupDriver() = default;

bool CgroupDriver::detect() {
  if (!is_cgroup2()) {
    return false;
  }

  std::string controllers_path = "/sys/fs/cgroup/cgroup.controllers";
  FILE* f = fopen(controllers_path.c_str(), "r");
  if (!f) {
    return false;
  }

  char buf[256];
  bool has_cpu = false;
  bool has_memory = false;
  bool has_pids = false;

  if (fgets(buf, sizeof(buf), f)) {
    char* tok = strtok(buf, " \n");
    while (tok) {
      if (strcmp(tok, "cpu") == 0) {
        has_cpu = true;
      } else if (strcmp(tok, "memory") == 0) {
        has_memory = true;
      } else if (strcmp(tok, "pids") == 0) {
        has_pids = true;
      }
      tok = strtok(nullptr, " \n");
    }
  }
  fclose(f);

  Capability caps = Capability::NONE;
  if (has_cpu) {
    caps = caps | Capability::CPU;
  }
  if (has_memory) {
    caps = caps | Capability::MEMORY;
  }
  if (has_pids) {
    caps = caps | Capability::PIDS;
  }

  capability_ = caps;

  if (caps == Capability::NONE) {
    return false;
  }

  return create_base_dir();
}

bool CgroupDriver::create_base_dir() {
  base_path_ = kDefaultCgroupPath;

  if (mkdir(base_path_.c_str(), 0755) != 0 && errno != EEXIST) {
    return false;
  }

  if (access(base_path_.c_str(), R_OK | W_OK) != 0) {
    return false;
  }

  return true;
}

CgroupDriver::ApplyResult CgroupDriver::apply(int32_t pid, const CpuPolicy& cpu,
                                              const MemPolicy& mem, const PidsPolicy& pids) {
  ApplyResult result;

  if (!available_) {
    result.success = true;
    return result;
  }

  std::stringstream ss;
  ss << base_path_ << "/" << pid;

  std::string pid_path = ss.str();
  if (mkdir(pid_path.c_str(), 0755) != 0 && errno != EEXIST) {
    result.err = errno;
    result.error_detail = "failed to create cgroup: " + std::string(strerror(errno));
    return result;
  }

  std::string procs_path = pid_path + "/cgroup.procs";
  int pf = open(procs_path.c_str(), O_WRONLY);
  if (pf < 0) {
    result.err = errno;
    result.error_detail = "failed to open cgroup.procs: " + std::string(strerror(errno));
    return result;
  }

  char pidbuf[32];
  int len = snprintf(pidbuf, sizeof(pidbuf), "%d", pid);
  ssize_t written = write(pf, pidbuf, len);
  close(pf);

  if (written != len) {
    result.err = errno;
    result.error_detail = "failed to write pid to cgroup.procs";
    return result;
  }

  if (cpu.max_pct && has_capability(capability_, Capability::CPU)) {
    std::string cpu_max_path = pid_path + "/cpu.max";
    int cf = open(cpu_max_path.c_str(), O_WRONLY);
    if (cf >= 0) {
      uint64_t quota = (*cpu.max_pct * kCpuPeriod) / 100;
      char buf[64];
      int len = snprintf(buf, sizeof(buf), "%llu %llu\n", static_cast<unsigned long long>(quota),
                         static_cast<unsigned long long>(kCpuPeriod));
      write(cf, buf, len);
      close(cf);
      result.applied = result.applied | Capability::CPU;
    }
  }

  if (mem.max_bytes && has_capability(capability_, Capability::MEMORY)) {
    std::string mem_max_path = pid_path + "/memory.max";
    int mf = open(mem_max_path.c_str(), O_WRONLY);
    if (mf >= 0) {
      char buf[32];
      int len =
          snprintf(buf, sizeof(buf), "%llu\n", static_cast<unsigned long long>(*mem.max_bytes));
      write(mf, buf, len);
      close(mf);
      result.applied = result.applied | Capability::MEMORY;
    }
  }

  if (pids.max && has_capability(capability_, Capability::PIDS)) {
    std::string pids_max_path = pid_path + "/pids.max";
    int pf = open(pids_max_path.c_str(), O_WRONLY);
    if (pf >= 0) {
      char buf[32];
      int len = snprintf(buf, sizeof(buf), "%u\n", *pids.max);
      write(pf, buf, len);
      close(pf);
      result.applied = result.applied | Capability::PIDS;
    }
  }

  result.success = true;
  return result;
}

void CgroupDriver::cleanup(int32_t pid) {
  if (!available_) {
    return;
  }

  std::stringstream ss;
  ss << base_path_ << "/" << pid;
  rmdir(ss.str().c_str());
}

} // namespace gov
} // namespace heidi
