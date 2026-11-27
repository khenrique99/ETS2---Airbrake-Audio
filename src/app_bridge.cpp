#include "app_bridge.h"

#include "command_inbox.h"
#include "log.h"
#include "settings_commands.h"
#include "shared/hotkey_combo.h"
#include "shared/ipc.h"
#include "status_writer.h"

#include <windows.h>

#include <chrono>

namespace aa {

namespace {
constexpr size_t kMaxLocalCommands = 64;
}

AppBridge::AppBridge(const Config &config, std::wstring iniPath, const Metrics &metrics, bool soundsLoaded,
                     std::string soundError)
    : metrics_(metrics), iniPath_(std::move(iniPath)), soundsLoaded_(soundsLoaded), soundError_(std::move(soundError)),
      shown_(config) {}

void AppBridge::start(std::function<void()> onHotkey) {
    if (running_) return;
    if (shown_.appEnabled && onHotkey) {
        hotkey::Combo combo;
        if (!hotkey::parse(shown_.hotkey, combo)) hotkey::parse(hotkey::kDefault, combo);
        hotkeys_.start(combo, std::move(onHotkey));
    }
    running_ = true;
    thread_ = std::thread(&AppBridge::run, this);
}

void AppBridge::stop() {
    hotkeys_.stop();
    running_ = false;
    if (thread_.joinable()) thread_.join();
}

bool AppBridge::takePending(Config &out) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!dirty_) return false;
    out = shown_;
    dirty_ = false;
    return true;
}

PanelSnapshot AppBridge::snapshot() const {
    PanelSnapshot s;
    s.paused = metrics_.paused.load();
    s.pedal = metrics_.pedal.load();
    s.speedKmh = metrics_.speedKmh.load();
    s.heldS = metrics_.heldS.load();
    s.sound = static_cast<SoundState>(metrics_.sound.load());
    s.soundsLoaded = soundsLoaded_;
    s.soundError = soundError_;
    std::lock_guard<std::mutex> lock(mutex_);
    s.config = shown_;
    s.unsaved = unsaved_;
    s.lastCommandOk = lastOk_;
    s.lastCommandResult = lastResult_;
    return s;
}

void AppBridge::submit(const std::string &action) {
    Command cmd;
    // O formato exige um numero de sequencia; o do painel e sempre 0 ("local", sem confirmacao em arquivo).
    if (!parseCommandLine("1 " + action, cmd)) {
        log::warn("Comando do painel recusado: '%s'", action.c_str());
        return;
    }
    cmd.seq = 0;
    std::lock_guard<std::mutex> lock(mutex_);
    if (localCommands_.size() < kMaxLocalCommands) localCommands_.push_back(std::move(cmd));
}

void AppBridge::run() {
    using namespace std::chrono_literals;
    while (running_) {
        handleCommands();
        const bool appOpen = FindWindowW(ipc::kAppWindowClass, nullptr) != nullptr;
        const unsigned interval = appOpen ? ipc::kLiveMs : ipc::kHeartbeatMs;
        if (GetTickCount64() - lastPublishMs_ >= interval) publish();
        std::this_thread::sleep_for(50ms);
    }
}

void AppBridge::handleCommands() {
    std::vector<Command> commands = takeCommands();
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (Command &c : localCommands_) commands.push_back(std::move(c));
        localCommands_.clear();
    }
    if (commands.empty()) return;

    for (const Command &cmd : commands) execute(cmd);
    lastPublishMs_ = 0; // responde ja
}

void AppBridge::execute(const Command &cmd) {
    if (cmd.seq != 0 && cmd.seq <= lastSeq_) return; // reenvio do mesmo comando do app

    CommandOutcome outcome;
    Config snapshot;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        outcome = interpretCommand(cmd, shown_);
        if (outcome.configChanged) {
            dirty_ = true;
            unsaved_ = true;
        }
        snapshot = shown_;
    }
    if (outcome.saveRequested) {
        outcome.ok = saveConfig(iniPath_, snapshot);
        outcome.message = outcome.ok ? "Ajustes salvos no airbrake_audio.ini."
                                     : "Nao consegui gravar o airbrake_audio.ini (pasta protegida?).";
        if (outcome.ok) {
            std::lock_guard<std::mutex> lock(mutex_);
            unsaved_ = false;
        }
    }
    if (outcome.reloadHotkey) {
        reloadHotkey();
        std::lock_guard<std::mutex> lock(mutex_);
        outcome.message = "Atalho: " + shown_.hotkey;
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        lastOk_ = outcome.ok;
        lastResult_ = outcome.message;
    }
    if (cmd.seq != 0) lastSeq_ = cmd.seq;
    if (!outcome.ok) log::warn("Comando '%s': %s", cmd.action.c_str(), outcome.message.c_str());
}

void AppBridge::reloadHotkey() {
    const Config fromFile = loadConfig(iniPath_);
    hotkey::Combo combo;
    if (!hotkey::parse(fromFile.hotkey, combo)) return;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        shown_.hotkey = fromFile.hotkey;
    }
    hotkeys_.setCombo(combo);
}

void AppBridge::publish() {
    const PanelSnapshot s = snapshot();
    StatusInfo info;
    info.config = &s.config;
    info.paused = s.paused;
    info.pedal = s.pedal;
    info.speedKmh = s.speedKmh;
    info.heldS = s.heldS;
    info.sound = s.sound;
    info.unsaved = s.unsaved;
    info.configPath = ipc::narrow(iniPath_);
    info.soundsLoaded = s.soundsLoaded;
    info.soundError = s.soundError;
    info.lastCommandSeq = lastSeq_;
    info.lastCommandOk = s.lastCommandOk;
    info.lastCommandResult = s.lastCommandResult;
    writeStatus(info);
    lastPublishMs_ = GetTickCount64();
}

} // namespace aa
