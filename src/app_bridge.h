#pragma once
// Ponte entre o jogo e o app AirbrakeAudio.exe, numa thread propria:
//   - le command.txt e aplica os comandos numa copia dos ajustes;
//   - grava status.json (batimento de 1 s; 10x por segundo com o app aberto);
//   - abre o app pelo atalho.
// A thread do jogo nunca espera por ela: publica Metrics (atomicos) e pega os ajustes novos
// com takePending().
#include "brake_sounds.h"
#include "config.h"
#include "hotkeys.h"

#include <atomic>
#include <mutex>
#include <string>
#include <thread>

namespace aa {

// Escrito pela thread do jogo, lido pela ponte.
struct Metrics {
    std::atomic<float> pedal{0.0f};
    std::atomic<float> speedKmh{0.0f};
    std::atomic<float> heldS{0.0f};
    std::atomic<int> sound{0}; // SoundState
    std::atomic<bool> paused{true};
};

class AppBridge {
public:
    AppBridge(const Config &config, std::wstring iniPath, const Metrics &metrics, bool soundsLoaded,
              std::string soundError);
    AppBridge(const AppBridge &) = delete;
    AppBridge &operator=(const AppBridge &) = delete;
    ~AppBridge() { stop(); }

    void start();
    void stop();

    // Thread do jogo: true quando o app mudou os ajustes desde a ultima chamada.
    bool takePending(Config &out);

private:
    void run();
    void handleCommands();
    void publish();
    void reloadHotkey();

    const Metrics &metrics_;
    const std::wstring iniPath_;
    const bool soundsLoaded_;
    const std::string soundError_;

    std::mutex mutex_; // protege shown_ e dirty_
    Config shown_;     // ajustes vistos pelo app; vao para o jogo via takePending
    bool dirty_ = false;

    // So a thread da ponte mexe nestes.
    bool unsaved_ = false;
    long long lastSeq_ = 0;
    bool lastOk_ = true;
    std::string lastResult_;
    unsigned long long lastPublishMs_ = 0;

    HotkeyListener hotkeys_;
    std::atomic<bool> running_{false};
    std::thread thread_;
};

} // namespace aa
