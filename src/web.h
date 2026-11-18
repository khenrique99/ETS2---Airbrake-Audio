// Settings page: a tiny HTTP server on 127.0.0.1 plus a hotkey.
//
// - A hotkey thread watches the configured key (default "-"), but only while the
//   game window is in the foreground. Pressing it starts the server (if needed)
//   and opens the page in a clean browser window.
// - The server stops by itself after a few minutes without requests, so no port
//   stays open while you are not configuring anything.
// - It only listens on 127.0.0.1 and rejects foreign Host/Origin headers, and
//   every POST needs a custom header, which keeps other web pages from driving it.
//
// The game thread never blocks on this code: it reads Metrics (atomics) and picks
// up a new Config through takePending().
#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <shellapi.h>
#include <atomic>
#include <cstdlib>
#include <cstdio>
#include <locale>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include "config.h"
#include "version.h"
#include "webpage.h"

// Written by the game thread, read by the server thread.
struct Metrics {
    std::atomic<float> pedal{0.0f};
    std::atomic<float> speedKmh{0.0f};
    std::atomic<float> heldS{0.0f};
    std::atomic<int>   state{0};        // 0 none, 1 brake, 2 air release
};

// Maps KeyboardEvent.code (what the page sends) to a Windows virtual-key code. 0 = unsupported.
inline int virtualKeyFromCode(const std::string& code) {
    if (code == "Minus") return VK_OEM_MINUS;
    if (code == "Equal") return VK_OEM_PLUS;
    if (code == "Backquote") return VK_OEM_3;
    if (code == "Semicolon") return VK_OEM_1;
    if (code == "Slash") return VK_OEM_2;
    if (code == "Backslash") return VK_OEM_5;
    if (code == "BracketLeft") return VK_OEM_4;
    if (code == "BracketRight") return VK_OEM_6;
    if (code == "Quote") return VK_OEM_7;
    if (code == "Comma") return VK_OEM_COMMA;
    if (code == "Period") return VK_OEM_PERIOD;
    if (code == "NumpadSubtract") return VK_SUBTRACT;
    if (code == "NumpadAdd") return VK_ADD;
    if (code == "Insert") return VK_INSERT;
    if (code == "Delete") return VK_DELETE;
    if (code == "Home") return VK_HOME;
    if (code == "End") return VK_END;
    if (code == "PageUp") return VK_PRIOR;
    if (code == "PageDown") return VK_NEXT;
    if (code.size() == 4 && code.compare(0, 3, "Key") == 0 && code[3] >= 'A' && code[3] <= 'Z') return code[3];
    if (code.size() == 6 && code.compare(0, 5, "Digit") == 0 && code[5] >= '0' && code[5] <= '9') return code[5];
    if (code.size() >= 2 && code.size() <= 3 && code[0] == 'F') {
        const int n = std::atoi(code.c_str() + 1);
        if (n >= 1 && n <= 24 && std::to_string(n) == code.substr(1)) return VK_F1 + n - 1;
    }
    return 0;
}

class WebUi {
public:
    ~WebUi() { stop(); }

    // Starts the hotkey watcher. Does nothing when [ui] web_ui=off.
    void start(const Config& config, const std::wstring& iniPath, Metrics* metrics) {
        iniPath_ = iniPath;
        metrics_ = metrics;
        shown_ = config;
        hotkeyVk_ = virtualKeyFromCode(config.hotkey);
        if (!config.webUi) return;
        stopping_ = false;
        hotkeyThread_ = std::thread([this] { hotkeyLoop(); });
    }

    void stop() {
        stopping_ = true;
        if (hotkeyThread_.joinable()) hotkeyThread_.join();
        std::lock_guard<std::mutex> guard(serverMutex_);
        if (serverThread_.joinable()) serverThread_.join();
    }

    // Game thread: true when the page changed the settings since the last call.
    bool takePending(Config& out) {
        if (!dirty_.load()) return false;
        std::lock_guard<std::mutex> guard(configMutex_);
        out = shown_;
        dirty_ = false;
        return true;
    }

    // Starts the server if needed and returns its port (0 on failure).
    int ensureServer() {
        std::lock_guard<std::mutex> guard(serverMutex_);
        if (!serverRunning_) {
            if (serverThread_.joinable()) serverThread_.join();
            port_ = 0;
            serverRunning_ = true;
            serverThread_ = std::thread([this] { serveLoop(); });
        }
        for (int i = 0; i < 100 && serverRunning_ && port_ == 0; ++i) Sleep(10);
        return serverRunning_ ? port_.load() : 0;
    }

    int port() const { return port_.load(); }

private:
    // ---- hotkey -----------------------------------------------------------------

    static bool gameIsForeground() {
        DWORD pid = 0;
        GetWindowThreadProcessId(GetForegroundWindow(), &pid);
        return pid == GetCurrentProcessId();
    }

    void hotkeyLoop() {
        bool wasDown = false;
        ULONGLONG lastOpen = 0;
        while (!stopping_) {
            Sleep(30);
            const int vk = hotkeyVk_.load();
            const bool down = vk != 0 && gameIsForeground() && (GetAsyncKeyState(vk) & 0x8000) != 0;
            if (down && !wasDown && GetTickCount64() - lastOpen > 700) {
                lastOpen = GetTickCount64();
                openPage();
            }
            wasDown = down;
        }
    }

    void openPage() {
        const int port = ensureServer();
        if (port == 0) return;
        const std::wstring url = L"http://127.0.0.1:" + std::to_wstring(port) + L"/";
        // A clean app-style window (Edge ships with Windows); fall back to the default browser.
        const std::wstring args = L"--app=" + url + L" --window-size=820,940";
        const HINSTANCE result = ShellExecuteW(nullptr, L"open", L"msedge.exe", args.c_str(), nullptr, SW_SHOWNORMAL);
        if (reinterpret_cast<INT_PTR>(result) <= 32) {
            ShellExecuteW(nullptr, L"open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        }
    }

    // ---- server -----------------------------------------------------------------

    static std::string numberText(double value) {
        std::ostringstream text;
        text.imbue(std::locale::classic());
        text << value;
        return text.str();
    }

    std::string stateJson() {
        Config config;
        {
            std::lock_guard<std::mutex> guard(configMutex_);
            config = shown_;
        }
        const Config defaults;
        std::string json = "{\"v\":\"" AIRBRAKE_AUDIO_VERSION "\",\"hotkey\":\"" + config.hotkey + "\",\"metrics\":{";
        json += "\"pedal\":" + numberText(metrics_->pedal.load()) +
                ",\"speed\":" + numberText(metrics_->speedKmh.load()) +
                ",\"held\":" + numberText(metrics_->heldS.load()) +
                ",\"state\":" + std::to_string(metrics_->state.load()) + "},\"fields\":[";
        size_t count = 0;
        const FloatField* fields = floatFields(count);
        for (size_t i = 0; i < count; ++i) {
            const std::string id = cfgdetail::toAscii(fields[i].section) + "." + cfgdetail::toAscii(fields[i].key);
            if (i) json += ",";
            json += "{\"id\":\"" + id + "\",\"min\":" + numberText(fields[i].low) +
                    ",\"max\":" + numberText(fields[i].high) +
                    ",\"value\":" + numberText(config.*(fields[i].member)) +
                    ",\"def\":" + numberText(defaults.*(fields[i].member)) + "}";
        }
        return json + "]}";
    }

    // Body: "id=value&id=value". Values are plain numbers or a key code, so no URL decoding is needed.
    void applyForm(const std::string& body) {
        std::lock_guard<std::mutex> guard(configMutex_);
        size_t count = 0;
        const FloatField* fields = floatFields(count);
        size_t pos = 0;
        while (pos < body.size()) {
            size_t end = body.find('&', pos);
            if (end == std::string::npos) end = body.size();
            const std::string pair = body.substr(pos, end - pos);
            pos = end + 1;
            const size_t eq = pair.find('=');
            if (eq == std::string::npos) continue;
            const std::string key = pair.substr(0, eq);
            const std::string value = pair.substr(eq + 1);

            if (key == "hotkey") {
                const int vk = virtualKeyFromCode(value);
                if (vk != 0) { shown_.hotkey = value; hotkeyVk_ = vk; }
                continue;
            }
            for (size_t i = 0; i < count; ++i) {
                const std::string id = cfgdetail::toAscii(fields[i].section) + "." + cfgdetail::toAscii(fields[i].key);
                if (id != key) continue;
                std::istringstream parse(value);
                parse.imbue(std::locale::classic());
                float number = 0.0f;
                if (parse >> number) shown_.*(fields[i].member) = number;
            }
        }
        sanitizeConfig(shown_);
        dirty_ = true;
    }

    void resetDefaults() {
        std::lock_guard<std::mutex> guard(configMutex_);
        const Config defaults;
        size_t count = 0;
        const FloatField* fields = floatFields(count);
        for (size_t i = 0; i < count; ++i) shown_.*(fields[i].member) = defaults.*(fields[i].member);
        dirty_ = true;
    }

    static std::string headerValue(const std::string& headers, const std::string& name) {
        std::string lower = headers;
        for (char& c : lower) c = char(tolower(static_cast<unsigned char>(c)));
        const std::string needle = "\r\n" + name + ":";
        const size_t at = lower.find(needle);
        if (at == std::string::npos) return "";
        size_t start = at + needle.size();
        const size_t end = headers.find("\r\n", start);
        std::string value = headers.substr(start, end == std::string::npos ? std::string::npos : end - start);
        const size_t first = value.find_first_not_of(" \t");
        return first == std::string::npos ? "" : value.substr(first);
    }

    static void sendAll(SOCKET client, const std::string& data) {
        size_t sent = 0;
        while (sent < data.size()) {
            const int n = send(client, data.data() + sent, int(data.size() - sent), 0);
            if (n <= 0) return;
            sent += size_t(n);
        }
    }

    static void respond(SOCKET client, int status, const char* type, const std::string& body) {
        const char* text = status == 200 ? "OK" : status == 403 ? "Forbidden" : "Not Found";
        std::string head = "HTTP/1.1 " + std::to_string(status) + " " + text + "\r\nContent-Type: " + type +
                           "\r\nContent-Length: " + std::to_string(body.size()) +
                           "\r\nCache-Control: no-store\r\nX-Content-Type-Options: nosniff\r\nConnection: close\r\n\r\n";
        sendAll(client, head + body);
    }

    void handle(SOCKET client) {
        DWORD timeoutMs = 2000;
        setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeoutMs), sizeof(timeoutMs));

        std::string request;
        char buffer[2048];
        size_t headerEnd = std::string::npos;
        while (headerEnd == std::string::npos && request.size() < 16384) {
            const int n = recv(client, buffer, sizeof(buffer), 0);
            if (n <= 0) return;
            request.append(buffer, size_t(n));
            headerEnd = request.find("\r\n\r\n");
        }
        if (headerEnd == std::string::npos) return;

        const std::string headers = request.substr(0, headerEnd + 2);   // keeps the CRLF of the last header
        const size_t lineEnd = headers.find("\r\n");
        const std::string line = headers.substr(0, lineEnd);
        const size_t sp1 = line.find(' ');
        const size_t sp2 = line.find(' ', sp1 + 1);
        if (sp1 == std::string::npos || sp2 == std::string::npos) return;
        const std::string method = line.substr(0, sp1);
        std::string path = line.substr(sp1 + 1, sp2 - sp1 - 1);
        const size_t query = path.find('?');
        if (query != std::string::npos) path.erase(query);

        // Only our own page may talk to us (blocks DNS rebinding and other sites).
        const std::string host = headerValue(headers, "host");
        const std::string own1 = "127.0.0.1:" + std::to_string(port_.load());
        const std::string own2 = "localhost:" + std::to_string(port_.load());
        if (host != own1 && host != own2) { respond(client, 403, "text/plain", "forbidden"); return; }
        const std::string origin = headerValue(headers, "origin");
        if (!origin.empty() && origin != "http://" + host) { respond(client, 403, "text/plain", "forbidden"); return; }

        std::string body = request.substr(headerEnd + 4);
        size_t contentLength = size_t(std::atoi(headerValue(headers, "content-length").c_str()));
        if (contentLength > 8192) { respond(client, 403, "text/plain", "too large"); return; }
        while (body.size() < contentLength) {
            const int n = recv(client, buffer, sizeof(buffer), 0);
            if (n <= 0) break;
            body.append(buffer, size_t(n));
        }

        lastRequest_ = GetTickCount64();

        if (method == "GET" && path == "/") {
            respond(client, 200, "text/html; charset=utf-8", std::string(webPageHead()) + webPageScript());
        } else if (method == "GET" && path == "/api/state") {
            respond(client, 200, "application/json", stateJson());
        } else if (method == "POST") {
            if (headerValue(headers, "x-airbrake") != "1") { respond(client, 403, "text/plain", "forbidden"); return; }
            if (path == "/api/config") {
                applyForm(body);
                respond(client, 200, "application/json", "{\"ok\":true}");
            } else if (path == "/api/reset") {
                resetDefaults();
                respond(client, 200, "application/json", "{\"ok\":true}");
            } else if (path == "/api/save") {
                Config copy;
                {
                    std::lock_guard<std::mutex> guard(configMutex_);
                    copy = shown_;
                }
                const bool ok = saveConfig(iniPath_, copy);
                respond(client, 200, "application/json", ok ? "{\"ok\":true}" : "{\"ok\":false}");
            } else {
                respond(client, 404, "text/plain", "not found");
            }
        } else {
            respond(client, 404, "text/plain", "not found");
        }
    }

    void serveLoop() {
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) { serverRunning_ = false; return; }

        SOCKET listener = INVALID_SOCKET;
        int basePort = 8770;
        {
            std::lock_guard<std::mutex> guard(configMutex_);
            basePort = int(shown_.webPort);
        }
        for (int offset = 0; offset < 10 && listener == INVALID_SOCKET; ++offset) {
            SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (s == INVALID_SOCKET) break;
            sockaddr_in address{};
            address.sin_family = AF_INET;
            address.sin_port = htons(u_short(basePort + offset));
            inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);   // loopback only
            if (bind(s, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0 && listen(s, 4) == 0) {
                listener = s;
                port_ = basePort + offset;
            } else {
                closesocket(s);
            }
        }
        if (listener == INVALID_SOCKET) {
            WSACleanup();
            serverRunning_ = false;
            return;
        }

        lastRequest_ = GetTickCount64();
        while (!stopping_ && GetTickCount64() - lastRequest_ < kIdleMs) {
            fd_set readSet;
            FD_ZERO(&readSet);
            FD_SET(listener, &readSet);
            timeval wait{0, 250 * 1000};
            if (select(0, &readSet, nullptr, nullptr, &wait) <= 0) continue;
            SOCKET client = accept(listener, nullptr, nullptr);
            if (client == INVALID_SOCKET) continue;
            handle(client);
            closesocket(client);
        }
        closesocket(listener);
        WSACleanup();
        port_ = 0;
        serverRunning_ = false;
    }

    static constexpr ULONGLONG kIdleMs = 3ULL * 60 * 1000;

    std::wstring iniPath_;
    Metrics* metrics_ = nullptr;

    std::mutex configMutex_;
    Config shown_;                       // what the page shows; sent to the game thread via takePending
    std::atomic<bool> dirty_{false};
    std::atomic<int> hotkeyVk_{0};

    std::atomic<bool> stopping_{false};
    std::atomic<bool> serverRunning_{false};
    std::atomic<int> port_{0};
    std::atomic<ULONGLONG> lastRequest_{0};
    std::mutex serverMutex_;
    std::thread hotkeyThread_;
    std::thread serverThread_;
};
