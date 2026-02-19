#include "heidi-kernel/gov_rule.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <sstream>

namespace heidi {
namespace gov {

namespace {

bool is_whitespace(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

bool is_digit(char c) {
  return c >= '0' && c <= '9';
}

bool is_alpha(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

void skip_whitespace(std::string_view& s) {
  while (!s.empty() && is_whitespace(s[0]))
    s.remove_prefix(1);
}

std::string_view trim(std::string_view s) {
  while (!s.empty() && is_whitespace(s.back()))
    s.remove_suffix(1);
  while (!s.empty() && is_whitespace(s.front()))
    s.remove_prefix(1);
  return s;
}

bool parse_string_value(std::string_view& s, std::string& out) {
  skip_whitespace(s);
  if (s.empty() || s[0] != '"')
    return false;
  s.remove_prefix(1);
  size_t end = 0;
  while (end < s.size() && s[end] != '"') {
    if (s[end] == '\\' && end + 1 < s.size())
      end += 2;
    else
      end++;
  }
  if (end >= s.size())
    return false;
  out = std::string(s.substr(0, end));
  s.remove_prefix(end + 1);
  return true;
}

bool parse_int_value(std::string_view& s, int64_t& out) {
  skip_whitespace(s);
  size_t start = 0;
  if (!s.empty() && s[0] == '-')
    start = 1;
  if (start >= s.size() || !is_digit(s[start]))
    return false;
  size_t end = start;
  while (end < s.size() && is_digit(s[end]))
    end++;
  out = std::stoll(std::string(s.substr(0, end)));
  s.remove_prefix(end);
  return true;
}

bool parse_uint_value(std::string_view& s, uint64_t& out) {
  skip_whitespace(s);
  if (s.empty() || !is_digit(s[0]))
    return false;
  size_t end = 0;
  while (end < s.size() && is_digit(s[end]))
    end++;
  out = std::stoull(std::string(s.substr(0, end)));
  s.remove_prefix(end);
  return true;
}

bool parse_uint8_value(std::string_view& s, uint8_t& out) {
  uint64_t val = 0;
  if (!parse_uint_value(s, val))
    return false;
  if (val > 255)
    return false;
  out = static_cast<uint8_t>(val);
  return true;
}

bool parse_int8_value(std::string_view& s, int8_t& out) {
  int64_t val = 0;
  if (!parse_int_value(s, val))
    return false;
  if (val < -128 || val > 127)
    return false;
  out = static_cast<int8_t>(val);
  return true;
}

bool parse_key(std::string_view& s, std::string& key) {
  skip_whitespace(s);
  if (s.empty() || s[0] != '"')
    return false;
  s.remove_prefix(1);
  size_t end = 0;
  while (end < s.size() && s[end] != '"') {
    if (s[end] == '\\' && end + 1 < s.size())
      end += 2;
    else
      end++;
  }
  if (end >= s.size())
    return false;
  key = std::string(s.substr(0, end));
  s.remove_prefix(end + 1);
  return true;
}

bool consume_colon(std::string_view& s) {
  skip_whitespace(s);
  if (!s.empty() && s[0] == ':') {
    s.remove_prefix(1);
    return true;
  }
  return false;
}

} // namespace

std::string ack_to_string(AckCode code) {
  switch (code) {
  case AckCode::ACK:
    return "ACK";
  case AckCode::NACK_INVALID_PAYLOAD:
    return "NACK_INVALID_PAYLOAD";
  case AckCode::NACK_INVALID_PID:
    return "NACK_INVALID_PID";
  case AckCode::NACK_INVALID_RANGE:
    return "NACK_INVALID_RANGE";
  case AckCode::NACK_PARSE_ERROR:
    return "NACK_PARSE_ERROR";
  case AckCode::NACK_UNKNOWN_FIELD:
    return "NACK_UNKNOWN_FIELD";
  case AckCode::NACK_QUEUE_FULL:
    return "NACK_QUEUE_FULL";
  case AckCode::NACK_PROCESS_DEAD:
    return "NACK_PROCESS_DEAD";
  }
  return "UNKNOWN";
}

ParseResult parse_gov_apply(std::string_view payload) {
  ParseResult result;

  if (payload.size() > kMaxPayloadSize) {
    result.ack = AckCode::NACK_INVALID_PAYLOAD;
    result.error_detail = "payload exceeds 512 bytes";
    return result;
  }
  if (payload.empty()) {
    result.ack = AckCode::NACK_INVALID_PAYLOAD;
    result.error_detail = "empty payload";
    return result;
  }

  std::string_view s = trim(payload);
  if (s.front() != '{' || s.back() != '}') {
    result.ack = AckCode::NACK_PARSE_ERROR;
    result.error_detail = "expected JSON object";
    return result;
  }
  s.remove_prefix(1);
  s.remove_suffix(1);
  s = trim(s);

  bool has_pid = false;
  bool has_cpu = false;
  bool has_mem = false;
  bool has_pids = false;
  bool has_rlim = false;
  bool has_oom_score_adj = false;

  while (!s.empty()) {
    std::string key;
    if (!parse_key(s, key)) {
      result.ack = AckCode::NACK_PARSE_ERROR;
      result.error_detail = "failed to parse key";
      return result;
    }
    if (!consume_colon(s)) {
      result.ack = AckCode::NACK_PARSE_ERROR;
      result.error_detail = "missing colon after key";
      return result;
    }

    if (key == "pid") {
      int64_t pid_val = 0;
      if (!parse_int_value(s, pid_val)) {
        result.ack = AckCode::NACK_PARSE_ERROR;
        result.error_detail = "failed to parse pid value";
        return result;
      }
      if (pid_val <= 0) {
        result.ack = AckCode::NACK_INVALID_PID;
        result.error_detail = "pid must be positive";
        return result;
      }
      result.msg.pid = static_cast<int32_t>(pid_val);
      has_pid = true;

    } else if (key == "cpu") {
      if (s.empty() || s.front() != '{') {
        result.ack = AckCode::NACK_PARSE_ERROR;
        result.error_detail = "cpu must be an object";
        return result;
      }
      size_t brace_end = 1;
      int depth = 1;
      while (brace_end < s.size() && depth > 0) {
        if (s[brace_end] == '{')
          depth++;
        else if (s[brace_end] == '}')
          depth--;
        brace_end++;
      }
      std::string_view cpu_obj = s.substr(0, brace_end);
      s.remove_prefix(brace_end);

      cpu_obj = trim(cpu_obj);
      cpu_obj.remove_prefix(1);
      cpu_obj.remove_suffix(1);
      cpu_obj = trim(cpu_obj);

      CpuPolicy cpu_policy;
      while (!cpu_obj.empty()) {
        std::string cpu_key;
        if (!parse_key(cpu_obj, cpu_key)) {
          result.ack = AckCode::NACK_PARSE_ERROR;
          result.error_detail = "failed to parse cpu sub-key";
          return result;
        }
        if (!consume_colon(cpu_obj)) {
          result.ack = AckCode::NACK_PARSE_ERROR;
          result.error_detail = "missing colon in cpu object";
          return result;
        }
        if (cpu_key == "affinity") {
          std::string val;
          if (!parse_string_value(cpu_obj, val)) {
            result.ack = AckCode::NACK_PARSE_ERROR;
            result.error_detail = "failed to parse affinity value";
            return result;
          }
          cpu_policy.affinity = val;
        } else if (cpu_key == "nice") {
          int8_t val;
          if (!parse_int8_value(cpu_obj, val)) {
            result.ack = AckCode::NACK_INVALID_RANGE;
            result.error_detail = "nice value out of range (-128 to 127)";
            return result;
          }
          cpu_policy.nice = val;
        } else if (cpu_key == "max_pct") {
          uint8_t val;
          if (!parse_uint8_value(cpu_obj, val)) {
            result.ack = AckCode::NACK_INVALID_RANGE;
            result.error_detail = "max_pct value out of range (0-255)";
            return result;
          }
          cpu_policy.max_pct = val;
        } else {
          result.ack = AckCode::NACK_UNKNOWN_FIELD;
          result.error_detail = "unknown cpu field: " + cpu_key;
          return result;
        }

        skip_whitespace(cpu_obj);
        if (!cpu_obj.empty() && cpu_obj[0] == ',') {
          cpu_obj.remove_prefix(1);
          cpu_obj = trim(cpu_obj);
        }
      }
      result.msg.cpu = cpu_policy;
      has_cpu = true;

    } else if (key == "mem") {
      if (s.empty() || s.front() != '{') {
        result.ack = AckCode::NACK_PARSE_ERROR;
        result.error_detail = "mem must be an object";
        return result;
      }
      size_t brace_end = 1;
      int depth = 1;
      while (brace_end < s.size() && depth > 0) {
        if (s[brace_end] == '{')
          depth++;
        else if (s[brace_end] == '}')
          depth--;
        brace_end++;
      }
      std::string_view mem_obj = s.substr(0, brace_end);
      s.remove_prefix(brace_end);

      mem_obj = trim(mem_obj);
      mem_obj.remove_prefix(1);
      mem_obj.remove_suffix(1);
      mem_obj = trim(mem_obj);

      MemPolicy mem_policy;
      while (!mem_obj.empty()) {
        std::string mem_key;
        if (!parse_key(mem_obj, mem_key)) {
          result.ack = AckCode::NACK_PARSE_ERROR;
          result.error_detail = "failed to parse mem sub-key";
          return result;
        }
        if (!consume_colon(mem_obj)) {
          result.ack = AckCode::NACK_PARSE_ERROR;
          result.error_detail = "missing colon in mem object";
          return result;
        }
        if (mem_key == "max_bytes") {
          uint64_t val;
          if (!parse_uint_value(mem_obj, val)) {
            result.ack = AckCode::NACK_PARSE_ERROR;
            result.error_detail = "failed to parse max_bytes value";
            return result;
          }
          mem_policy.max_bytes = val;
        } else {
          result.ack = AckCode::NACK_UNKNOWN_FIELD;
          result.error_detail = "unknown mem field: " + mem_key;
          return result;
        }

        skip_whitespace(mem_obj);
        if (!mem_obj.empty() && mem_obj[0] == ',') {
          mem_obj.remove_prefix(1);
          mem_obj = trim(mem_obj);
        }
      }
      result.msg.mem = mem_policy;
      has_mem = true;

    } else if (key == "pids") {
      if (s.empty() || s.front() != '{') {
        result.ack = AckCode::NACK_PARSE_ERROR;
        result.error_detail = "pids must be an object";
        return result;
      }
      size_t brace_end = 1;
      int depth = 1;
      while (brace_end < s.size() && depth > 0) {
        if (s[brace_end] == '{')
          depth++;
        else if (s[brace_end] == '}')
          depth--;
        brace_end++;
      }
      std::string_view pids_obj = s.substr(0, brace_end);
      s.remove_prefix(brace_end);

      pids_obj = trim(pids_obj);
      pids_obj.remove_prefix(1);
      pids_obj.remove_suffix(1);
      pids_obj = trim(pids_obj);

      PidsPolicy pids_policy;
      while (!pids_obj.empty()) {
        std::string pids_key;
        if (!parse_key(pids_obj, pids_key)) {
          result.ack = AckCode::NACK_PARSE_ERROR;
          result.error_detail = "failed to parse pids sub-key";
          return result;
        }
        if (!consume_colon(pids_obj)) {
          result.ack = AckCode::NACK_PARSE_ERROR;
          result.error_detail = "missing colon in pids object";
          return result;
        }
        if (pids_key == "max") {
          uint64_t val;
          if (!parse_uint_value(pids_obj, val)) {
            result.ack = AckCode::NACK_PARSE_ERROR;
            result.error_detail = "failed to parse pids max value";
            return result;
          }
          if (val > 0xFFFFFFFF) {
            result.ack = AckCode::NACK_INVALID_RANGE;
            result.error_detail = "pids max out of range";
            return result;
          }
          pids_policy.max = static_cast<uint32_t>(val);
        } else {
          result.ack = AckCode::NACK_UNKNOWN_FIELD;
          result.error_detail = "unknown pids field: " + pids_key;
          return result;
        }

        skip_whitespace(pids_obj);
        if (!pids_obj.empty() && pids_obj[0] == ',') {
          pids_obj.remove_prefix(1);
          pids_obj = trim(pids_obj);
        }
      }
      result.msg.pids = pids_policy;
      has_pids = true;

    } else if (key == "rlim") {
      if (s.empty() || s.front() != '{') {
        result.ack = AckCode::NACK_PARSE_ERROR;
        result.error_detail = "rlim must be an object";
        return result;
      }
      size_t brace_end = 1;
      int depth = 1;
      while (brace_end < s.size() && depth > 0) {
        if (s[brace_end] == '{')
          depth++;
        else if (s[brace_end] == '}')
          depth--;
        brace_end++;
      }
      std::string_view rlim_obj = s.substr(0, brace_end);
      s.remove_prefix(brace_end);

      rlim_obj = trim(rlim_obj);
      rlim_obj.remove_prefix(1);
      rlim_obj.remove_suffix(1);
      rlim_obj = trim(rlim_obj);

      RlimPolicy rlim_policy;
      while (!rlim_obj.empty()) {
        std::string rlim_key;
        if (!parse_key(rlim_obj, rlim_key)) {
          result.ack = AckCode::NACK_PARSE_ERROR;
          result.error_detail = "failed to parse rlim sub-key";
          return result;
        }
        if (!consume_colon(rlim_obj)) {
          result.ack = AckCode::NACK_PARSE_ERROR;
          result.error_detail = "missing colon in rlim object";
          return result;
        }
        if (rlim_key == "nofile_soft") {
          uint64_t val;
          if (!parse_uint_value(rlim_obj, val)) {
            result.ack = AckCode::NACK_PARSE_ERROR;
            result.error_detail = "failed to parse nofile_soft value";
            return result;
          }
          rlim_policy.nofile_soft = val;
        } else if (rlim_key == "nofile_hard") {
          uint64_t val;
          if (!parse_uint_value(rlim_obj, val)) {
            result.ack = AckCode::NACK_PARSE_ERROR;
            result.error_detail = "failed to parse nofile_hard value";
            return result;
          }
          rlim_policy.nofile_hard = val;
        } else if (rlim_key == "core_soft") {
          uint64_t val;
          if (!parse_uint_value(rlim_obj, val)) {
            result.ack = AckCode::NACK_PARSE_ERROR;
            result.error_detail = "failed to parse core_soft value";
            return result;
          }
          rlim_policy.core_soft = val;
        } else if (rlim_key == "core_hard") {
          uint64_t val;
          if (!parse_uint_value(rlim_obj, val)) {
            result.ack = AckCode::NACK_PARSE_ERROR;
            result.error_detail = "failed to parse core_hard value";
            return result;
          }
          rlim_policy.core_hard = val;
        } else {
          result.ack = AckCode::NACK_UNKNOWN_FIELD;
          result.error_detail = "unknown rlim field: " + rlim_key;
          return result;
        }

        skip_whitespace(rlim_obj);
        if (!rlim_obj.empty() && rlim_obj[0] == ',') {
          rlim_obj.remove_prefix(1);
          rlim_obj = trim(rlim_obj);
        }
      }
      result.msg.rlim = rlim_policy;
      has_rlim = true;

    } else if (key == "oom_score_adj") {
      int64_t val = 0;
      if (!parse_int_value(s, val)) {
        result.ack = AckCode::NACK_PARSE_ERROR;
        result.error_detail = "failed to parse oom_score_adj value";
        return result;
      }
      if (val < -1000 || val > 1000) {
        result.ack = AckCode::NACK_INVALID_RANGE;
        result.error_detail = "oom_score_adj out of range (-1000 to 1000)";
        return result;
      }
      result.msg.oom_score_adj = static_cast<int>(val);
      has_oom_score_adj = true;

    } else {
      result.ack = AckCode::NACK_UNKNOWN_FIELD;
      result.error_detail = "unknown field: " + key;
      return result;
    }

    skip_whitespace(s);
    if (!s.empty() && s[0] == ',') {
      s.remove_prefix(1);
      s = trim(s);
    }
  }

  if (!has_pid) {
    result.ack = AckCode::NACK_INVALID_PAYLOAD;
    result.error_detail = "missing required field: pid";
    return result;
  }

  result.success = true;
  result.ack = AckCode::ACK;
  return result;
}

} // namespace gov
} // namespace heidi
