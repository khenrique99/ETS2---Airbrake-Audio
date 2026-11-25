#include "command_inbox.h"

#include "shared/ipc.h"

#include <sstream>

namespace aa {

namespace {

constexpr size_t kMaxWord = 32;
constexpr size_t kMaxArgs = 2;
constexpr size_t kMaxCommandsPerRead = 32;

bool isActionChar(char c) { return (c >= 'a' && c <= 'z') || c == '_'; }

bool isArgChar(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '.' || c == '_' ||
           c == '+' || c == '-';
}

template <typename Pred> bool allOf(const std::string &s, Pred pred) {
    if (s.empty() || s.size() > kMaxWord) return false;
    for (char c : s)
        if (!pred(c)) return false;
    return true;
}

} // namespace

bool parseCommandLine(const std::string &line, Command &out) {
    std::istringstream ss(line);
    Command c;
    if (!(ss >> c.seq >> c.action) || c.seq <= 0) return false;
    if (!allOf(c.action, isActionChar)) return false;
    std::string arg;
    while (ss >> arg) {
        if (c.args.size() == kMaxArgs || !allOf(arg, isArgChar)) return false;
        c.args.push_back(arg);
    }
    out = std::move(c);
    return true;
}

std::vector<Command> takeCommands() {
    std::vector<Command> out;
    static const std::wstring path = ipc::commandPath();
    if (path.empty() || GetFileAttributesW(path.c_str()) == INVALID_FILE_ATTRIBUTES) return out;

    // "Pega" o arquivo atomicamente; se o app estiver trocando agora, tenta no proximo ciclo.
    const std::wstring taken = path + L".taken";
    if (!MoveFileExW(path.c_str(), taken.c_str(), MOVEFILE_REPLACE_EXISTING)) return out;

    std::string text;
    ipc::readAll(taken, text);
    DeleteFileW(taken.c_str());

    std::istringstream lines(text);
    std::string line;
    while (std::getline(lines, line) && out.size() < kMaxCommandsPerRead) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        Command c;
        if (parseCommandLine(line, c)) out.push_back(std::move(c));
    }
    return out;
}

} // namespace aa
