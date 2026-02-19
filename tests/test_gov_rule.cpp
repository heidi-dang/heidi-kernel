#include "heidi-kernel/gov_rule.h"

#include <gtest/gtest.h>

namespace heidi {
namespace gov {

class GovApplyParserTest : public ::testing::Test {};

TEST_F(GovApplyParserTest, ParseSimplePid) {
  auto result = parse_gov_apply(R"({"pid":1234})");
  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.msg.pid, 1234);
}

TEST_F(GovApplyParserTest, ParsePidWithCpuAffinity) {
  auto result = parse_gov_apply(R"({"pid":1234,"cpu":{"affinity":"0-3"}})");
  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.msg.pid, 1234);
  ASSERT_TRUE(result.msg.cpu.has_value());
  ASSERT_TRUE(result.msg.cpu->affinity.has_value());
  EXPECT_EQ(*result.msg.cpu->affinity, "0-3");
}

TEST_F(GovApplyParserTest, ParsePidWithCpuNice) {
  auto result = parse_gov_apply(R"({"pid":1234,"cpu":{"nice":10}})");
  EXPECT_TRUE(result.success);
  ASSERT_TRUE(result.msg.cpu.has_value());
  ASSERT_TRUE(result.msg.cpu->nice.has_value());
  EXPECT_EQ(*result.msg.cpu->nice, 10);
}

TEST_F(GovApplyParserTest, ParsePidWithCpuMaxPct) {
  auto result = parse_gov_apply(R"({"pid":1234,"cpu":{"max_pct":80}})");
  EXPECT_TRUE(result.success);
  ASSERT_TRUE(result.msg.cpu.has_value());
  ASSERT_TRUE(result.msg.cpu->max_pct.has_value());
  EXPECT_EQ(*result.msg.cpu->max_pct, 80);
}

TEST_F(GovApplyParserTest, ParsePidWithMemMaxBytes) {
  auto result = parse_gov_apply(R"({"pid":1234,"mem":{"max_bytes":8589934592}})");
  EXPECT_TRUE(result.success);
  ASSERT_TRUE(result.msg.mem.has_value());
  ASSERT_TRUE(result.msg.mem->max_bytes.has_value());
  EXPECT_EQ(*result.msg.mem->max_bytes, 8589934592ULL);
}

TEST_F(GovApplyParserTest, ParsePidWithPidsMax) {
  auto result = parse_gov_apply(R"({"pid":1234,"pids":{"max":256}})");
  EXPECT_TRUE(result.success);
  ASSERT_TRUE(result.msg.pids.has_value());
  ASSERT_TRUE(result.msg.pids->max.has_value());
  EXPECT_EQ(*result.msg.pids->max, 256);
}

TEST_F(GovApplyParserTest, ParsePidWithRlimNofile) {
  auto result = parse_gov_apply(R"({"pid":1234,"rlim":{"nofile_soft":1024,"nofile_hard":4096}})");
  EXPECT_TRUE(result.success);
  ASSERT_TRUE(result.msg.rlim.has_value());
  ASSERT_TRUE(result.msg.rlim->nofile_soft.has_value());
  ASSERT_TRUE(result.msg.rlim->nofile_hard.has_value());
  EXPECT_EQ(*result.msg.rlim->nofile_soft, 1024ULL);
  EXPECT_EQ(*result.msg.rlim->nofile_hard, 4096ULL);
}

TEST_F(GovApplyParserTest, ParsePidWithOomScoreAdj) {
  auto result = parse_gov_apply(R"({"pid":1234,"oom_score_adj":500})");
  EXPECT_TRUE(result.success);
  ASSERT_TRUE(result.msg.oom_score_adj.has_value());
  EXPECT_EQ(*result.msg.oom_score_adj, 500);
}

TEST_F(GovApplyParserTest, ParseFullPolicy) {
  auto result =
      parse_gov_apply(R"({"pid":1234,"cpu":{"affinity":"0-3","nice":10,"max_pct":80},)"
                      R"("mem":{"max_bytes":8589934592},"pids":{"max":256},)"
                      R"("rlim":{"nofile_soft":1024,"nofile_hard":4096},"oom_score_adj":500})");
  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.msg.pid, 1234);
}

TEST_F(GovApplyParserTest, RejectMissingPid) {
  auto result = parse_gov_apply(R"({"cpu":{"affinity":"0-3"}})");
  EXPECT_FALSE(result.success);
  EXPECT_EQ(result.ack, AckCode::NACK_INVALID_PAYLOAD);
}

TEST_F(GovApplyParserTest, RejectNegativePid) {
  auto result = parse_gov_apply(R"({"pid":-1})");
  EXPECT_FALSE(result.success);
  EXPECT_EQ(result.ack, AckCode::NACK_INVALID_PID);
}

TEST_F(GovApplyParserTest, RejectZeroPid) {
  auto result = parse_gov_apply(R"({"pid":0})");
  EXPECT_FALSE(result.success);
  EXPECT_EQ(result.ack, AckCode::NACK_INVALID_PID);
}

TEST_F(GovApplyParserTest, RejectUnknownField) {
  auto result = parse_gov_apply(R"({"pid":1234,"unknown_field":true})");
  EXPECT_FALSE(result.success);
  EXPECT_EQ(result.ack, AckCode::NACK_UNKNOWN_FIELD);
}

TEST_F(GovApplyParserTest, RejectUnknownCpuField) {
  auto result = parse_gov_apply(R"({"pid":1234,"cpu":{"unknown":true}})");
  EXPECT_FALSE(result.success);
  EXPECT_EQ(result.ack, AckCode::NACK_UNKNOWN_FIELD);
}

TEST_F(GovApplyParserTest, RejectInvalidNiceRange) {
  auto result = parse_gov_apply(R"({"pid":1234,"cpu":{"nice":200}})");
  EXPECT_FALSE(result.success);
  EXPECT_EQ(result.ack, AckCode::NACK_INVALID_RANGE);
}

TEST_F(GovApplyParserTest, RejectInvalidOomScoreAdjRange) {
  auto result = parse_gov_apply(R"({"pid":1234,"oom_score_adj":2000})");
  EXPECT_FALSE(result.success);
  EXPECT_EQ(result.ack, AckCode::NACK_INVALID_RANGE);
}

TEST_F(GovApplyParserTest, RejectOversizedPayload) {
  std::string big_payload(600, 'x');
  big_payload = "{\"pid\":1234,\"data\":\"" + big_payload + "\"}";
  auto result = parse_gov_apply(big_payload);
  EXPECT_FALSE(result.success);
  EXPECT_EQ(result.ack, AckCode::NACK_INVALID_PAYLOAD);
}

TEST_F(GovApplyParserTest, RejectInvalidJson) {
  auto result = parse_gov_apply("not json");
  EXPECT_FALSE(result.success);
  EXPECT_EQ(result.ack, AckCode::NACK_PARSE_ERROR);
}

TEST_F(GovApplyParserTest, AcceptsTrailingComma) {
  auto result = parse_gov_apply(R"({"pid":1234,})");
  EXPECT_TRUE(result.success);
}

TEST_F(GovApplyParserTest, AckCodeToString) {
  EXPECT_EQ(ack_to_string(AckCode::ACK), "ACK");
  EXPECT_EQ(ack_to_string(AckCode::NACK_INVALID_PAYLOAD), "NACK_INVALID_PAYLOAD");
  EXPECT_EQ(ack_to_string(AckCode::NACK_INVALID_PID), "NACK_INVALID_PID");
  EXPECT_EQ(ack_to_string(AckCode::NACK_INVALID_RANGE), "NACK_INVALID_RANGE");
  EXPECT_EQ(ack_to_string(AckCode::NACK_PARSE_ERROR), "NACK_PARSE_ERROR");
  EXPECT_EQ(ack_to_string(AckCode::NACK_UNKNOWN_FIELD), "NACK_UNKNOWN_FIELD");
  EXPECT_EQ(ack_to_string(AckCode::NACK_QUEUE_FULL), "NACK_QUEUE_FULL");
  EXPECT_EQ(ack_to_string(AckCode::NACK_PROCESS_DEAD), "NACK_PROCESS_DEAD");
  EXPECT_EQ(ack_to_string(AckCode::NACK_INVALID_GROUP), "NACK_INVALID_GROUP");
  EXPECT_EQ(ack_to_string(AckCode::NACK_GROUP_CAPACITY), "NACK_GROUP_CAPACITY");
}

TEST_F(GovApplyParserTest, ParseV2WithGroup) {
  auto result =
      parse_gov_apply(R"({"version":"v2","pid":1234,"group":"mygroup","cpu":{"affinity":"0-1"}})");
  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.msg.version, GovVersion::V2);
  ASSERT_TRUE(result.msg.group.has_value());
  EXPECT_EQ(*result.msg.group, "mygroup");
}

TEST_F(GovApplyParserTest, ParseV2WithAction) {
  auto result = parse_gov_apply(R"({"version":"v2","pid":1234,"action":"warn"})");
  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.msg.version, GovVersion::V2);
  ASSERT_TRUE(result.msg.action.has_value());
  EXPECT_EQ(*result.msg.action, ViolationAction::WARN);
}

TEST_F(GovApplyParserTest, ParseV2WithCpuPeriodUs) {
  auto result = parse_gov_apply(R"({"version":"v2","pid":1234,"cpu":{"period_us":10000}})");
  EXPECT_TRUE(result.success);
  ASSERT_TRUE(result.msg.cpu.has_value());
  ASSERT_TRUE(result.msg.cpu->period_us.has_value());
  EXPECT_EQ(*result.msg.cpu->period_us, 10000);
}

TEST_F(GovApplyParserTest, ParseV2WithMemHighBytes) {
  auto result = parse_gov_apply(
      R"({"version":"v2","pid":1234,"mem":{"max_bytes":8589934592,"high_bytes":4294967296}})");
  EXPECT_TRUE(result.success);
  ASSERT_TRUE(result.msg.mem.has_value());
  ASSERT_TRUE(result.msg.mem->max_bytes.has_value());
  ASSERT_TRUE(result.msg.mem->high_bytes.has_value());
  EXPECT_EQ(*result.msg.mem->max_bytes, 8589934592ULL);
  EXPECT_EQ(*result.msg.mem->high_bytes, 4294967296ULL);
}

TEST_F(GovApplyParserTest, ParseV1BackwardCompat) {
  auto result = parse_gov_apply(R"({"pid":1234,"cpu":{"affinity":"0-3"}})");
  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.msg.version, GovVersion::V1);
>>>>>>> 33a8683 (feat(gov): initial process resource governor (P1-1 + P1-2 core))
>>>>>>> ec0e7ce (feat(gov): initial process resource governor (P1-1 + P1-2 core))
}

} // namespace gov
} // namespace heidi
