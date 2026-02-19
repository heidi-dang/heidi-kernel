#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>

namespace heidi {
namespace gov {

constexpr size_t kMaxPayloadSize = 512;
constexpr size_t kMaxCpus = 128;
constexpr size_t kMaxGroupIdLen = 32;

enum class AckCode : uint8_t {
  ACK = 0,
  NACK_INVALID_PAYLOAD = 1,
  NACK_INVALID_PID = 2,
  NACK_INVALID_RANGE = 3,
  NACK_PARSE_ERROR = 4,
  NACK_UNKNOWN_FIELD = 5,
  NACK_QUEUE_FULL = 6,
  NACK_PROCESS_DEAD = 7,
  NACK_INVALID_GROUP = 8,
  NACK_GROUP_CAPACITY = 9,
};

enum class GovVersion : uint8_t {
  V1 = 1,
  V2 = 2,
};

enum class ViolationAction : uint8_t {
  NONE = 0,
  WARN = 1,
  SOFT_KILL = 2,
  HARD_KILL = 3,
};

struct CpuPolicy {
  std::optional<std::string> affinity;
  std::optional<int8_t> nice;
  std::optional<uint8_t> max_pct;
  std::optional<uint32_t> period_us;
};

struct MemPolicy {
  std::optional<uint64_t> max_bytes;
  std::optional<uint64_t> high_bytes;
};

struct PidsPolicy {
  std::optional<uint32_t> max;
};

struct RlimPolicy {
  std::optional<uint64_t> nofile_soft;
  std::optional<uint64_t> nofile_hard;
  std::optional<uint64_t> core_soft;
  std::optional<uint64_t> core_hard;
};

struct TimeoutPolicy {
  std::optional<uint32_t> apply_deadline_ms;
};

struct GovApplyMsg {
  GovVersion version = GovVersion::V1;
  int32_t pid = 0;
  std::optional<std::string> group;
  std::optional<ViolationAction> action;
  std::optional<CpuPolicy> cpu;
  std::optional<MemPolicy> mem;
  std::optional<PidsPolicy> pids;
  std::optional<RlimPolicy> rlim;
  std::optional<int> oom_score_adj;
  std::optional<TimeoutPolicy> timeouts;
};

struct ParseResult {
  bool success = false;
  AckCode ack = AckCode::ACK;
  GovApplyMsg msg;
  std::string error_detail;
};

ParseResult parse_gov_apply(std::string_view payload);

std::string ack_to_string(AckCode code);

enum class ApplyField : uint16_t {
  NONE = 0,
  CPU_AFFINITY = 1 << 0,
  CPU_NICE = 1 << 1,
  CPU_MAX_PCT = 1 << 2,
  CPU_PERIOD_US = 1 << 3,
  MEM_MAX_BYTES = 1 << 4,
  MEM_HIGH_BYTES = 1 << 5,
  PIDS_MAX = 1 << 6,
  RLIM_NOFILE = 1 << 7,
  RLIM_CORE = 1 << 8,
  OOM_SCORE_ADJ = 1 << 9,
  GROUP = 1 << 10,
  ACTION = 1 << 11,
  TIMEOUT_APPLY_DEADLINE_MS = 1 << 12,
};

constexpr ApplyField operator|(ApplyField a, ApplyField b) {
  return static_cast<ApplyField>(static_cast<uint16_t>(a) | static_cast<uint16_t>(b));
}
constexpr bool has_field(ApplyField fields, ApplyField field) {
  return (static_cast<uint16_t>(fields) & static_cast<uint16_t>(field)) != 0;
}

} // namespace gov
} // namespace heidi
