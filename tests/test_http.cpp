#include "heidi-kernel/http.h"

#include <gtest/gtest.h>

using namespace heidi;

TEST(HttpServerTest, ParseValidRequest) {
  std::string raw = "GET /api/status HTTP/1.1\r\n"
                    "Host: localhost\r\n"
                    "User-Agent: test\r\n"
                    "\r\n";
  HttpRequest req = HttpServer::parse_request(raw);

  EXPECT_EQ(req.method, "GET");
  EXPECT_EQ(req.path, "/api/status");
  EXPECT_EQ(req.headers["Host"], "localhost");
  EXPECT_EQ(req.headers["User-Agent"], "test");
}

TEST(HttpServerTest, ParseRequestWithBody) {
  std::string raw = "POST /api/governor/policy HTTP/1.1\r\n"
                    "Content-Length: 15\r\n"
                    "\r\n"
                    "{\"foo\":\"bar\"}";
  HttpRequest req = HttpServer::parse_request(raw);

  EXPECT_EQ(req.method, "POST");
  EXPECT_EQ(req.path, "/api/governor/policy");
  EXPECT_EQ(req.body, "{\"foo\":\"bar\"}");
  EXPECT_EQ(req.headers["Content-Length"], "15");
}

TEST(HttpServerTest, ParseMalformedRequestLine) {
  std::string raw = "INVALID\r\n\r\n";
  HttpRequest req = HttpServer::parse_request(raw);
  EXPECT_TRUE(req.method.empty());
}

TEST(HttpServerTest, ParseIncompleteRequest) {
  std::string raw = "GET /";
  HttpRequest req = HttpServer::parse_request(raw);
  EXPECT_TRUE(req.method.empty());
}

TEST(HttpServerTest, ParseHeadersWithSpaces) {
  std::string raw = "GET / HTTP/1.1\r\n"
                    "Key:   Value  \r\n"
                    "\r\n";
  HttpRequest req = HttpServer::parse_request(raw);
  EXPECT_EQ(req.method, "GET");
  EXPECT_EQ(req.headers["Key"], "Value");
}

TEST(HttpServerTest, ParseNoHeaders) {
  std::string raw = "GET / HTTP/1.1\r\n\r\n";
  HttpRequest req = HttpServer::parse_request(raw);
  EXPECT_EQ(req.method, "GET");
  EXPECT_TRUE(req.headers.empty());
}
