#include <gtest/gtest.h>
#include "heidi-kernel/http.h"

using namespace heidi;

TEST(HttpParserTest, BasicGetRequest) {
    std::string raw = "GET /index.html HTTP/1.1\r\nHost: localhost\r\n\r\n";
    HttpRequest req = HttpServer::parse_request(raw);

    EXPECT_EQ(req.method, "GET");
    EXPECT_EQ(req.path, "/index.html");
    EXPECT_TRUE(req.body.empty());
    // Note: Headers parsing is not implemented in parse_request yet
    EXPECT_TRUE(req.headers.empty());
}

TEST(HttpParserTest, PostRequestWithBody) {
    std::string raw = "POST /api/data HTTP/1.1\r\nContent-Length: 15\r\n\r\n{\"status\":\"ok\"}";
    HttpRequest req = HttpServer::parse_request(raw);

    EXPECT_EQ(req.method, "POST");
    EXPECT_EQ(req.path, "/api/data");
    EXPECT_EQ(req.body, "{\"status\":\"ok\"}");
}

TEST(HttpParserTest, RequestWithWhitespace) {
    // Tests trimming logic
    std::string raw = "  DELETE   /resource   HTTP/1.1\r\n\r\n";
    HttpRequest req = HttpServer::parse_request(raw);

    EXPECT_EQ(req.method, "DELETE");
    EXPECT_EQ(req.path, "/resource");
}

TEST(HttpParserTest, EmptyRequest) {
    std::string raw = "";
    HttpRequest req = HttpServer::parse_request(raw);

    EXPECT_TRUE(req.method.empty());
    EXPECT_TRUE(req.path.empty());
    EXPECT_TRUE(req.body.empty());
}

TEST(HttpParserTest, IncompleteRequestLine) {
    std::string raw = "GET\r\n\r\n";
    HttpRequest req = HttpServer::parse_request(raw);

    // Should fail to parse method/path because splitting by space fails
    EXPECT_TRUE(req.method.empty());
    EXPECT_TRUE(req.path.empty());
}

TEST(HttpParserTest, OnlyBody) {
    // If no \r\n found, nothing parsed?
    // Code says: size_t line_end = data.find("\r\n");
    // If not found, request line logic skipped.
    std::string raw = "Just some text";
    HttpRequest req = HttpServer::parse_request(raw);

    EXPECT_TRUE(req.method.empty());
    EXPECT_TRUE(req.path.empty());
    // Body logic: size_t pos = data.find("\r\n\r\n");
    // If not found, body_view is empty.
    EXPECT_TRUE(req.body.empty());
}

TEST(HttpParserTest, BodyWithoutHeadersSeparator) {
    // What if \r\n exists but not \r\n\r\n?
    std::string raw = "GET / HTTP/1.1\r\nHost: localhost";
    HttpRequest req = HttpServer::parse_request(raw);

    EXPECT_EQ(req.method, "GET");
    EXPECT_EQ(req.path, "/");
    EXPECT_TRUE(req.body.empty());
}

TEST(HttpParserTest, EmptyBodyWithSeparator) {
    std::string raw = "GET / HTTP/1.1\r\n\r\n";
    HttpRequest req = HttpServer::parse_request(raw);

    EXPECT_EQ(req.method, "GET");
    EXPECT_EQ(req.path, "/");
    EXPECT_TRUE(req.body.empty());
}
