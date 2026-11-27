#pragma once
// Ponte entre o jogo e a tela (painel dentro do jogo ou AirbrakeAudio.exe), numa thread propria:
//   - le command.txt (app externo) e os comandos do painel e aplica numa copia dos ajustes;
//   - grava status.json (batimento de 1 s; 10x por segundo com o app aberto);
//   - chama o atalho (abre o painel / o app).
// A thread do jogo nunca espera por ela: publica Metrics (atomicos) e pega os ajustes novos
// com takePending().
#include "brake_sounds.h"
#include "command_inbox.h"
#include "config.h"
#include "hotkeys.h"
#include "overlay/panel_host.h"

#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace aa {

// Escrito pela thread do jogo, lido pela ponte.
struct Metrics {
    std::atomic<float> pedal{0.0f};
    std::atomic<float> speedKmh{0.0f};
    std::atomic<float> heldS{0.0f};
    std::atomic<int> sound{0}; // SoundState
    std::atomic<bool> paused{true};
};

class AppBridge final : public PanelHost {
public:
    AppBridge(const Config &config, std::wstring iniPath, const Metrics &metrics, bool soundsLoaded,
              std::string soundError);
    AppBridge(const AppBridge &) = delete;
    AppBridge &operator=(const AppBridge &) = delete;
    ~AppBridge() override { stop(); }

    // onHotkey roda na thread do teclado (so deve abrir/fechar a tela).
    void start(std::function<void()> onHotkey);
    void stop();

    // Thread do jogo: true quando a tela mudou os ajustes desde a ultima chamada.
    bool takePending(Config &out);

    // PanelHost (qualquer thread)
    PanelSnapshot snapshot() const override;
    void submit(const std::string &action) override;

private:
    void run();
    void handleCommands();
    void execute(const Command &cmd);
    void publish();
    void reloadHotkey();

    const Metrics &metrics_;
    const std::wstring iniPath_;
    const bool soundsLoaded_;
    const std::string soundError_;

    mutable std::mutex mutex_; // protege tudo desta secao
    Config shown_;             // ajustes vistos pela tela; vao para o jogo via takePending
    bool dirty_ = false;
    bool unsaved_ = false;
    bool lastOk_ = true;
    std::string lastResult_;
    std::vector<Command> localCommands_; // vindos do painel (seq = 0)

    // So a thread da ponte mexe nestes.
    long long lastSeq_ = 0;
    unsigned long long lastPublishMs_ = 0;

    HotkeyListener hotkeys_;
    std::atomic<bool> running_{false};
    std::thread thread_;
};

} // namespace aa
