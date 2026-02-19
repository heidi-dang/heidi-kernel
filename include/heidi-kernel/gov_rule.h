#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>

namespace heidi {
namespace gov {

constexpr size_t kMaxPayloadSize = 512;
constexpr size_t kMaxCpus = 128;

enum class AckCode : uint8_t {
  ACK = 0,
  NACK_INVALID_PAYLOAD = 1,
  NACK_INVALID_PID = 2,
  NACK_INVALID_RANGE = 3,
  NACK_PARSE_ERROR = 4,
  NACK_UNKNOWN_FIELD = 5,
  NACK_QUEUE_FULL = 6,
  NACK_PROCESS_DEAD = 7,
};

struct CpuPolicy {
  std::optional<std::string> affinity;
  std::optional<int8_t> nice;
  std::optional<uint8_t> max_pct;
};

struct MemPolicy {
  std::optional<uint64_t> max_bytes;
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

struct GovApplyMsg {
  int32_t pid = 0;
  std::optional<CpuPolicy> cpu;
  std::optional<MemPolicy> mem;
  std::optional<PidsPolicy> pids;
  std::optional<RlimPolicy> rlim;
  std::optional<int> oom_score_adj;
};

struct ParseResult {
  bool success = false;
  AckCode ack = AckCode::ACK;
  GovApplyMsg msg;
  std::string error_detail;
};

ParseResult parse_gov_apply(std::string_view payload);

std::string ack_to_string(AckCode code);

enum class ApplyField : uint8_t {
  NONE = 0,
  CPU_AFFINITY = 1 << 0,
  CPU_NICE = 1 << 1,
  CPU_MAX_PCT = 1 << 2,
  MEM_MAX_BYTES = 1 << 3,
  PIDS_MAX = 1 << 4,
  RLIM_NOFILE = 1 << 5,
  RLIM_CORE = 1 << 6,
  OOM_SCORE_ADJ = 1 << 7,
};

constexpr ApplyField operator|(ApplyField a, ApplyField b) {
  return static_cast<ApplyField>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
constexpr bool has_field(ApplyField fields, ApplyField field) {
  return (static_cast<uint8_t>(fields) & static_cast<uint8_t>(field)) != 0;
}

} // namespace gov
} // namespace heidi
