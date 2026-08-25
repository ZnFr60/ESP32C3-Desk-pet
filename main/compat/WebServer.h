#pragma once
// WebServer.h - Arduino WebServer over ESP-IDF native esp_http_server.
// HTTP_GET / HTTP_POST / HTTP_ANY constants come from esp_http_server.h.
#include <stdint.h>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <string.h>
#include "esp_http_server.h"
#include "WString.h"

class WebServer {
public:
    typedef std::function<void()> THandlerFunction;

    explicit WebServer(uint16_t port = 80) : m_port(port), m_server(nullptr), m_curReq(nullptr) {}

    void on(const char *uri, httpd_method_t method, THandlerFunction fn) {
        for (auto &r : m_routes) if (r.uri == uri && r.method == method) return; // dedupe on re-entry
        m_routes.push_back({ uri, method, fn });
    }
    void on(const String &uri, httpd_method_t method, THandlerFunction fn) {
        on(uri.c_str(), method, fn);
    }

    void begin() {
        if (m_server) return;
        httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
        cfg.server_port = m_port;
        cfg.stack_size = 8192;
        cfg.max_uri_handlers = 12;
        if (httpd_start(&m_server, &cfg) == ESP_OK) {
            for (auto &r : m_routes) {
                httpd_uri_t u = {};
                u.uri = r.uri.c_str();
                u.method = r.method;
                u.handler = &routeHandler;
                RouteCtx *ctx = new RouteCtx{ this, r.fn };
                u.user_ctx = ctx;
                r.reg = u;
                httpd_register_uri_handler(m_server, &r.reg);
            }
        }
    }

    // No-op: the httpd server runs in its own task and invokes handlers directly.
    void handleClient() {}

    void send(int code, const char *contentType, const char *content) {
        if (!m_curReq) return;
        httpd_resp_set_status(m_curReq, httpd_status(code));
        if (contentType) httpd_resp_set_type(m_curReq, contentType);
        if (content) httpd_resp_send(m_curReq, content, HTTPD_RESP_USE_STRLEN);
        else httpd_resp_send(m_curReq, nullptr, 0);
    }
    void send(int code, const char *contentType, const String &content) {
        send(code, contentType, content.c_str());
    }

    bool hasArg(const char *name) { return m_args.count(name) > 0; }
    String arg(const char *name) {
        auto it = m_args.find(name);
        return (it != m_args.end()) ? String(it->second.c_str()) : String();
    }

private:
    struct RouteCtx { WebServer *ws; THandlerFunction fn; };
    struct Route {
        std::string uri;
        httpd_method_t method;
        THandlerFunction fn;
        httpd_uri_t reg;
    };
    std::vector<Route> m_routes;
    uint16_t m_port;
    httpd_handle_t m_server;
    httpd_req_t *m_curReq;
    std::map<std::string, std::string> m_args;

    static const char *httpd_status(int code) {
        switch (code) {
            case 200: return "200 OK";
            case 400: return "400 Bad Request";
            case 404: return "404 Not Found";
            case 500: return "500 Internal Server Error";
            default:  return "200 OK";
        }
    }

    static void urlDecode(const std::string &in, std::string &out) {
        out.clear();
        for (size_t i = 0; i < in.size(); i++) {
            if (in[i] == '%' && i + 2 < in.size()) {
                char hi = in[i + 1], lo = in[i + 2];
                auto hex = [](char c) -> int { if (c >= '0' && c <= '9') return c - '0'; c |= 0x20; if (c >= 'a' && c <= 'f') return c - 'a' + 10; return 0; };
                out += (char)(hex(hi) * 16 + hex(lo));
                i += 2;
            } else if (in[i] == '+') {
                out += ' ';
            } else {
                out += in[i];
            }
        }
    }

    void parsePairs(const std::string &qs) {
        m_args.clear();
        size_t start = 0;
        while (start <= qs.size()) {
            size_t amp = qs.find('&', start);
            if (amp == std::string::npos) amp = qs.size();
            std::string pair = qs.substr(start, amp - start);
            size_t eq = pair.find('=');
            std::string k = pair.substr(0, eq), v = (eq != std::string::npos) ? pair.substr(eq + 1) : "";
            std::string dk, dv;
            urlDecode(k, dk); urlDecode(v, dv);
            m_args[dk] = dv;
            if (amp == qs.size()) break;
            start = amp + 1;
        }
    }

    void parseArgs(httpd_req_t *req) {
        std::string qs;
        const char *q = strchr(req->uri, '?');
        if (q && q[1]) qs = q + 1;
        // For POST forms, read the body.
        if (req->method == HTTP_POST) {
            char buf[512];
            int r = httpd_req_recv(req, buf, sizeof(buf) - 1);
            if (r > 0) { buf[r] = 0; if (!qs.empty()) qs += "&"; qs += buf; }
        }
        parsePairs(qs);
    }

    static esp_err_t routeHandler(httpd_req_t *req) {
        RouteCtx *ctx = (RouteCtx *)req->user_ctx;
        if (!ctx) return ESP_OK;
        ctx->ws->m_curReq = req;
        ctx->ws->parseArgs(req);
        ctx->fn();
        ctx->ws->m_curReq = nullptr;
        return ESP_OK;
    }
};