#include "heidi-kernel/http.h"
#include <gtest/gtest.h>

using namespace heidi;

TEST(HttpServerTest, ParseRequest_ValidGet) {
    std::string data = "GET /api/status HTTP/1.1\r\nHost: localhost\r\n\r\n";
    HttpRequest req = HttpServer::parse_request(data);

    EXPECT_EQ(req.method, "GET");
    EXPECT_EQ(req.path, "/api/status");
    EXPECT_TRUE(req.body.empty());
}

TEST(HttpServerTest, ParseRequest_ValidPostWithBody) {
    std::string data = "POST /submit HTTP/1.1\r\nContent-Length: 5\r\n\r\nhello";
    HttpRequest req = HttpServer::parse_request(data);

    EXPECT_EQ(req.method, "POST");
    EXPECT_EQ(req.path, "/submit");
    EXPECT_EQ(req.body, "hello");
}

TEST(HttpServerTest, ParseRequest_Incomplete) {
    std::string data = "GET /"; // Missing CRLFs
    HttpRequest req = HttpServer::parse_request(data);

    EXPECT_TRUE(req.method.empty());
    EXPECT_TRUE(req.path.empty());
    EXPECT_TRUE(req.body.empty());
}

TEST(HttpServerTest, ParseRequest_Lifetime) {
    // Ensure that if we pass a string_view that outlives the HttpRequest, it works.
    std::string data = "GET /safe HTTP/1.1\r\n\r\n";
    HttpRequest req = HttpServer::parse_request(data);

    EXPECT_EQ(req.path, "/safe");

    // Note: If we passed a temporary string to parse_request, usage here would be UB.
    // But since we changed signature to string_view, it forces caller to manage lifetime
    // of the underlying string data if it comes from a temporary string.
    // But here 'data' lives until end of scope.
}

TEST(HttpServerTest, FormatResponse_Simple) {
    HttpResponse resp;
    resp.status_code = 200;
    resp.status_text = "OK";
    resp.body = "hello";
    resp.headers["Content-Type"] = "text/plain";

    std::string s = HttpServer::format_response(resp);

    EXPECT_NE(s.find("HTTP/1.1 200 OK\r\n"), std::string::npos);
    EXPECT_NE(s.find("Content-Type: text/plain\r\n"), std::string::npos);
    EXPECT_NE(s.find("Content-Length: 5\r\n"), std::string::npos);
    // Check end of headers and body
    EXPECT_NE(s.find("\r\n\r\nhello"), std::string::npos);
}

TEST(HttpServerTest, FormatResponse_404) {
    HttpResponse resp;
    resp.status_code = 404;
    resp.status_text = "Not Found";

    std::string s = HttpServer::format_response(resp);

    EXPECT_NE(s.find("HTTP/1.1 404 Not Found\r\n"), std::string::npos);
    EXPECT_NE(s.find("Content-Length: 0\r\n"), std::string::npos);
}
