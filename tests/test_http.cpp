#include <gtest/gtest.h>
#include "heidi-kernel/http.h"

using namespace heidi;

TEST(HttpServerTest, ParseSimpleRequest) {
    std::string raw = "GET /index.html HTTP/1.1\r\n\r\n";
    HttpRequest req = HttpServer::parse_request(raw);

    EXPECT_EQ(req.method, "GET");
    EXPECT_EQ(req.path, "/index.html");
    EXPECT_TRUE(req.body.empty());
    EXPECT_TRUE(req.headers.empty());
}

TEST(HttpServerTest, ParseRequestWithHeaders) {
    std::string raw = "POST /api/v1/data HTTP/1.1\r\n"
                      "Host: localhost\r\n"
                      "Content-Type: application/json\r\n"
                      "\r\n";
    HttpRequest req = HttpServer::parse_request(raw);

    EXPECT_EQ(req.method, "POST");
    EXPECT_EQ(req.path, "/api/v1/data");
    EXPECT_EQ(req.headers.size(), 2);
    EXPECT_EQ(req.headers["Host"], "localhost");
    EXPECT_EQ(req.headers["Content-Type"], "application/json");
}

TEST(HttpServerTest, ParseRequestWithBody) {
    std::string raw = "POST /submit HTTP/1.1\r\n"
                      "Content-Length: 11\r\n"
                      "\r\n"
                      "hello world";
    HttpRequest req = HttpServer::parse_request(raw);

    EXPECT_EQ(req.method, "POST");
    EXPECT_EQ(req.body, "hello world");
    EXPECT_EQ(req.headers["Content-Length"], "11");
}

TEST(HttpServerTest, ParseMalformedRequestLine) {
    // Missing version (one space)
    std::string raw = "GET /\r\n\r\n";
    HttpRequest req = HttpServer::parse_request(raw);
    EXPECT_TRUE(req.method.empty());
    EXPECT_TRUE(req.path.empty());

    // Missing spaces entirely
    raw = "GET\r\n\r\n";
    req = HttpServer::parse_request(raw);
    EXPECT_TRUE(req.method.empty());
}

TEST(HttpServerTest, ParseMalformedHeaders) {
    // Header without colon
    std::string raw = "GET / HTTP/1.1\r\nInvalidHeader\r\n\r\n";
    HttpRequest req = HttpServer::parse_request(raw);
    EXPECT_EQ(req.method, "GET");
    EXPECT_TRUE(req.headers.empty());

    // Header with empty value
    raw = "GET / HTTP/1.1\r\nEmpty:\r\n\r\n";
    req = HttpServer::parse_request(raw);
    EXPECT_EQ(req.headers["Empty"], "");
}

TEST(HttpServerTest, IncompleteRequest) {
    // No double CRLF
    std::string raw = "GET / HTTP/1.1\r\nHost: foo";
    HttpRequest req = HttpServer::parse_request(raw);
    EXPECT_TRUE(req.method.empty());
}

TEST(HttpServerTest, HeadersWhitespace) {
    std::string raw = "GET / HTTP/1.1\r\nKey:   value  \r\n\r\n";
    HttpRequest req = HttpServer::parse_request(raw);
    EXPECT_EQ(req.headers["Key"], "value");
}

TEST(HttpServerTest, EmptyRequest) {
    HttpRequest req = HttpServer::parse_request("");
    EXPECT_TRUE(req.method.empty());
}
