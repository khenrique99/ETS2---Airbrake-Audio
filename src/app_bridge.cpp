#include "app_bridge.h"

#include "app_launcher.h"
#include "command_inbox.h"
#include "log.h"
#include "settings_commands.h"
#include "shared/hotkey_combo.h"
#include "shared/ipc.h"
#include "status_writer.h"

#include <windows.h>

#include <chrono>

namespace aa {

AppBridge::AppBridge(const Config &config, std::wstring iniPath, const Metrics &metrics, bool soundsLoaded,
                     std::string soundError)
    : metrics_(metrics), iniPath_(std::move(iniPath)), soundsLoaded_(soundsLoaded), soundError_(std::move(soundError)),
      shown_(config) {}

void AppBridge::start() {
    if (running_) return;
    if (shown_.appEnabled) {
        hotkey::Combo combo;
        if (!hotkey::parse(shown_.hotkey, combo)) hotkey::parse(hotkey::kDefault, combo);
        hotkeys_.start(combo, [] { log::info("%s", showSettingsApp().c_str()); });
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
    const std::vector<Command> commands = takeCommands();
    if (commands.empty()) return;

    for (const Command &cmd : commands) {
        if (cmd.seq <= lastSeq_) continue; // reenvio do mesmo comando
        CommandOutcome outcome;
        Config snapshot;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            outcome = interpretCommand(cmd, shown_);
            if (outcome.configChanged) dirty_ = true;
            snapshot = shown_;
        }
        if (outcome.configChanged) unsaved_ = true;
        if (outcome.saveRequested) {
            outcome.ok = saveConfig(iniPath_, snapshot);
            outcome.message = outcome.ok ? "Ajustes salvos no airbrake_audio.ini."
                                         : "Nao consegui gravar o airbrake_audio.ini (pasta protegida?).";
            if (outcome.ok) unsaved_ = false;
        }
        if (outcome.reloadHotkey) {
            reloadHotkey();
            std::lock_guard<std::mutex> lock(mutex_);
            outcome.message = "Atalho: " + shown_.hotkey;
        }
        lastSeq_ = cmd.seq;
        lastOk_ = outcome.ok;
        lastResult_ = outcome.message;
        if (!outcome.ok) log::warn("Comando '%s': %s", cmd.action.c_str(), outcome.message.c_str());
    }
    lastPublishMs_ = 0; // responde ao app ja
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
    StatusInfo info;
    Config snapshot;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        snapshot = shown_;
    }
    info.config = &snapshot;
    info.paused = metrics_.paused.load();
    info.pedal = metrics_.pedal.load();
    info.speedKmh = metrics_.speedKmh.load();
    info.heldS = metrics_.heldS.load();
    info.sound = static_cast<SoundState>(metrics_.sound.load());
    info.unsaved = unsaved_;
    info.configPath = ipc::narrow(iniPath_);
    info.soundsLoaded = soundsLoaded_;
    info.soundError = soundError_;
    info.lastCommandSeq = lastSeq_;
    info.lastCommandOk = lastOk_;
    info.lastCommandResult = lastResult_;
    writeStatus(info);
    lastPublishMs_ = GetTickCount64();
}

} // namespace aa
