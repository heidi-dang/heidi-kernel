#include "heidi-kernel/ipc.h"
#include <gtest/gtest.h>

namespace heidi {
namespace {

TEST(IpcProtocolTest, SerializePing) {
    IpcMessage msg;
    msg.type = "ping";
    std::string serialized = IpcProtocol::serialize(msg);
    EXPECT_EQ(serialized, "ping\n");
}

TEST(IpcProtocolTest, DeserializePong) {
    std::string data = "pong\n";
    IpcMessage msg = IpcProtocol::deserialize(data);
    EXPECT_EQ(msg.type, "pong");
}

} // namespace
} // namespace heidi