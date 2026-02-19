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
}

TEST_F(GovApplyParserTest, ParseV1BackwardCompat) {
  auto result = parse_gov_apply(R"({"pid":1234,"cpu":{"affinity":"0-3"}})");
  EXPECT_TRUE(result.success);
  // P1 API: payloads without explicit version are treated as V1; no version field present.
}
}

} // namespace gov
} // namespace heidi
