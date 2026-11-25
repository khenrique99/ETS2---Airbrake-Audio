#pragma once
#include "shared/hotkey_combo.h"

#include <atomic>
#include <functional>
#include <thread>

namespace aa {

// Le o teclado numa thread propria e chama o callback quando o atalho e apertado com a janela
// do jogo em foco. Os modificadores precisam bater exatamente (Ctrl + - do Trailer Control
// nao dispara Ctrl + =, e vice-versa). O callback roda nessa thread.
class HotkeyListener {
public:
    using Callback = std::function<void()>;

    HotkeyListener() = default;
    HotkeyListener(const HotkeyListener &) = delete;
    HotkeyListener &operator=(const HotkeyListener &) = delete;
    ~HotkeyListener() { stop(); }

    void start(const hotkey::Combo &combo, Callback cb);
    void stop();
    // Troca o atalho com a thread rodando (qualquer thread).
    void setCombo(const hotkey::Combo &combo);

private:
    void run();

    std::atomic<unsigned> packed_{0}; // vk | ctrl<<16 | shift<<17 | alt<<18
    Callback cb_;
    std::atomic<bool> running_{false};
    std::thread thread_;
};

} // namespace aa
