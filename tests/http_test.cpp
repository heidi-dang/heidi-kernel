#include "heidi-kernel/http.h"
#include <gtest/gtest.h>

using namespace heidi;

// Test fixture that creates an HttpServer instance
class HttpServerTest : public ::testing::Test {
protected:
    HttpServer server{"127.0.0.1", 0};  // Port 0 = let OS assign
};

TEST_F(HttpServerTest, ParseRequest_ValidGet) {
    std::string data = "GET /api/status HTTP/1.1\r\nHost: localhost\r\n\r\n";
    HttpRequest req = server.parse_request(data);

    EXPECT_EQ(std::string(req.method), "GET");
    EXPECT_EQ(std::string(req.path), "/api/status");
    EXPECT_TRUE(req.body.empty());
}

TEST_F(HttpServerTest, ParseRequest_ValidPostWithBody) {
    std::string data = "POST /submit HTTP/1.1\r\nContent-Length: 5\r\n\r\nhello";
    HttpRequest req = server.parse_request(data);

    EXPECT_EQ(std::string(req.method), "POST");
    EXPECT_EQ(std::string(req.path), "/submit");
    EXPECT_EQ(std::string(req.body), "hello");
}

TEST_F(HttpServerTest, ParseRequest_Incomplete) {
    std::string data = "GET /"; // Missing CRLFs
    HttpRequest req = server.parse_request(data);

    EXPECT_TRUE(req.method.empty());
    EXPECT_TRUE(req.path.empty());
    EXPECT_TRUE(req.body.empty());
}

TEST_F(HttpServerTest, ParseRequest_Empty) {
    std::string data = "";
    HttpRequest req = server.parse_request(data);

    EXPECT_TRUE(req.method.empty());
    EXPECT_TRUE(req.path.empty());
}

TEST_F(HttpServerTest, FormatResponse_Simple) {
    HttpResponse resp;
    resp.status_code = 200;
    resp.status_text = "OK";
    resp.body = "hello";
    resp.headers["Content-Type"] = "text/plain";

    std::string s = server.format_response(resp);

    EXPECT_NE(s.find("HTTP/1.1 200 OK\r\n"), std::string::npos);
    EXPECT_NE(s.find("Content-Type: text/plain\r\n"), std::string::npos);
    EXPECT_NE(s.find("Content-Length: 5\r\n"), std::string::npos);
    // Check end of headers and body
    EXPECT_NE(s.find("\r\n\r\nhello"), std::string::npos);
}

TEST_F(HttpServerTest, FormatResponse_404) {
    HttpResponse resp;
    resp.status_code = 404;
    resp.status_text = "Not Found";

    std::string s = server.format_response(resp);

    EXPECT_NE(s.find("HTTP/1.1 404 Not Found\r\n"), std::string::npos);
    EXPECT_NE(s.find("Content-Length: 0\r\n"), std::string::npos);
}

TEST_F(HttpServerTest, FormatResponse_WithHeaders) {
    HttpResponse resp;
    resp.status_code = 201;
    resp.status_text = "Created";
    resp.body = "{\"id\": 123}";
    resp.headers["Content-Type"] = "application/json";
    resp.headers["Location"] = "/items/123";

    std::string s = server.format_response(resp);

    EXPECT_NE(s.find("HTTP/1.1 201 Created\r\n"), std::string::npos);
    EXPECT_NE(s.find("Content-Type: application/json\r\n"), std::string::npos);
    EXPECT_NE(s.find("Location: /items/123\r\n"), std::string::npos);
    EXPECT_NE(s.find("\r\n\r\n{\"id\": 123}"), std::string::npos);
}

TEST_F(HttpServerTest, ParseRequest_PathWithQuery) {
    std::string data = "GET /api/status?verbose=true HTTP/1.1\r\n\r\n";
    HttpRequest req = server.parse_request(data);

    EXPECT_EQ(std::string(req.method), "GET");
    EXPECT_EQ(std::string(req.path), "/api/status?verbose=true");
}

TEST_F(HttpServerTest, ParseRequest_MethodCasePreserved) {
    std::string data = "post /data HTTP/1.1\r\n\r\n";
    HttpRequest req = server.parse_request(data);

    EXPECT_EQ(std::string(req.method), "post");  // Lowercase preserved
    EXPECT_EQ(std::string(req.path), "/data");
}
