#include <gtest/gtest.h>
#include "heidi-kernel/http.h"

using namespace heidi;

TEST(HttpParserTest, ParseSimpleGet) {
    std::string data = "GET /status HTTP/1.1\r\n\r\n";
    HttpRequest req = HttpServer::parse_request(data);
    EXPECT_EQ(req.method, "GET");
    EXPECT_EQ(req.path, "/status");
    EXPECT_TRUE(req.body.empty());
}

TEST(HttpParserTest, ParseWithBody) {
    std::string data = "POST /api/data HTTP/1.1\r\nContent-Length: 12\r\n\r\nHello World!";
    HttpRequest req = HttpServer::parse_request(data);
    EXPECT_EQ(req.method, "POST");
    EXPECT_EQ(req.path, "/api/data");
    EXPECT_EQ(req.body, "Hello World!");
}

TEST(HttpParserTest, MalformedRequestLine) {
    // Missing version
    std::string data = "GET /status\r\n\r\n";
    HttpRequest req = HttpServer::parse_request(data);
    EXPECT_TRUE(req.method.empty());
    EXPECT_TRUE(req.path.empty());

    // No spaces
    data = "GET\r\n\r\n";
    req = HttpServer::parse_request(data);
    EXPECT_TRUE(req.method.empty());
}

TEST(HttpParserTest, Vulnerability_ExtraData) {
    // Pipelining or smuggling
    std::string data = "POST / HTTP/1.1\r\nContent-Length: 5\r\n\r\nHelloExtra";
    HttpRequest req = HttpServer::parse_request(data);
    // Should be "Hello" but current implementation takes "HelloExtra"
    EXPECT_EQ(req.body, "Hello");
}

TEST(HttpParserTest, HeadersParsing) {
    std::string data = "GET / HTTP/1.1\r\nHost: localhost\r\nX-Custom: Value\r\n\r\n";
    HttpRequest req = HttpServer::parse_request(data);
    EXPECT_EQ(req.headers.size(), 2);
    EXPECT_EQ(req.headers["Host"], "localhost");
    EXPECT_EQ(req.headers["X-Custom"], "Value");
}

TEST(HttpParserTest, ContentLengthCaseInsensitive) {
    std::string data = "POST /api/data HTTP/1.1\r\ncontent-length: 12\r\n\r\nHello World!";
    HttpRequest req = HttpServer::parse_request(data);
    EXPECT_EQ(req.method, "POST");
    EXPECT_EQ(req.body, "Hello World!");
}
