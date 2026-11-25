#include "hotkeys.h"

#include <windows.h>

#include <chrono>

namespace aa {

namespace {

constexpr unsigned kCtrlBit = 1u << 16, kShiftBit = 1u << 17, kAltBit = 1u << 18;

unsigned pack(const hotkey::Combo &c) {
    return c.vk | (c.ctrl ? kCtrlBit : 0) | (c.shift ? kShiftBit : 0) | (c.alt ? kAltBit : 0);
}

bool down(int vk) { return (GetAsyncKeyState(vk) & 0x8000) != 0; }

bool gameHasFocus() {
    HWND fg = GetForegroundWindow();
    if (!fg) return false;
    DWORD pid = 0;
    GetWindowThreadProcessId(fg, &pid);
    return pid == GetCurrentProcessId();
}

// "-" e "=" valem tambem no teclado numerico.
int numpadTwin(unsigned vk) {
    if (vk == VK_OEM_MINUS) return VK_SUBTRACT;
    if (vk == VK_OEM_PLUS) return VK_ADD;
    return 0;
}

} // namespace

void HotkeyListener::start(const hotkey::Combo &combo, Callback cb) {
    if (running_ || !combo.valid()) return;
    setCombo(combo);
    cb_ = std::move(cb);
    running_ = true;
    thread_ = std::thread(&HotkeyListener::run, this);
}

void HotkeyListener::stop() {
    running_ = false;
    if (thread_.joinable()) thread_.join();
}

void HotkeyListener::setCombo(const hotkey::Combo &combo) {
    if (combo.valid()) packed_ = pack(combo);
}

void HotkeyListener::run() {
    using namespace std::chrono_literals;
    bool wasDown = false;
    while (running_) {
        const unsigned p = packed_.load();
        const int vk = static_cast<int>(p & 0xFFFF);
        const int twin = numpadTwin(static_cast<unsigned>(vk));
        const bool modifiersMatch = down(VK_CONTROL) == ((p & kCtrlBit) != 0) &&
                                    down(VK_SHIFT) == ((p & kShiftBit) != 0) && down(VK_MENU) == ((p & kAltBit) != 0);
        const bool isDown = modifiersMatch && (down(vk) || (twin && down(twin))) && gameHasFocus();
        if (isDown && !wasDown && cb_) cb_();
        wasDown = isDown;
        std::this_thread::sleep_for(15ms);
    }
}

} // namespace aa
