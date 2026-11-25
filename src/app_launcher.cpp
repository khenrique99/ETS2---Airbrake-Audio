#include "app_launcher.h"

#include "config.h"
#include "shared/ipc.h"

#include <windows.h>

#include <cstdint>
#include <cstdio>

namespace aa {

namespace {

// Janela principal visivel deste processo (o jogo).
HWND findGameWindow() {
    struct Ctx {
        DWORD pid;
        HWND found;
    } ctx{GetCurrentProcessId(), nullptr};
    EnumWindows(
        [](HWND h, LPARAM lp) -> BOOL {
            auto *c = reinterpret_cast<Ctx *>(lp);
            DWORD pid = 0;
            GetWindowThreadProcessId(h, &pid);
            if (pid == c->pid && IsWindowVisible(h) && GetWindow(h, GW_OWNER) == nullptr) {
                c->found = h;
                return FALSE;
            }
            return TRUE;
        },
        reinterpret_cast<LPARAM>(&ctx));
    HWND fg = GetForegroundWindow();
    DWORD pid = 0;
    if (fg) GetWindowThreadProcessId(fg, &pid);
    return pid == ctx.pid ? fg : ctx.found;
}

} // namespace

std::string showSettingsApp() {
    HWND game = findGameWindow();
    if (HWND app = FindWindowW(ipc::kAppWindowClass, nullptr)) {
        // O jogo esta em primeiro plano, entao pode passar o foco para o app.
        PostMessageW(app, ipc::kMsgShowFromGame, reinterpret_cast<WPARAM>(game), 0);
        ShowWindow(app, IsIconic(app) ? SW_RESTORE : SW_SHOW);
        SetForegroundWindow(app);
        return "Tela do Airbrake Audio aberta.";
    }

    const std::wstring dir = pluginDirectory();
    const std::wstring exe = dir + L"AirbrakeAudio.exe";
    if (GetFileAttributesW(exe.c_str()) == INVALID_FILE_ATTRIBUTES)
        return "AirbrakeAudio.exe nao esta na pasta plugins (junto da DLL); copie-o para la.";

    wchar_t cmd[1024];
    swprintf_s(cmd, L"\"%s\" --from-game %llu", exe.c_str(),
               static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(game)));
    STARTUPINFOW si{sizeof(si)};
    PROCESS_INFORMATION pi{};
    if (!CreateProcessW(exe.c_str(), cmd, nullptr, nullptr, FALSE, 0, nullptr, dir.c_str(), &si, &pi)) {
        char buf[96];
        std::snprintf(buf, sizeof(buf), "Nao consegui abrir o AirbrakeAudio.exe (erro %lu).", GetLastError());
        return buf;
    }
    AllowSetForegroundWindow(pi.dwProcessId);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return "Abrindo a tela do Airbrake Audio.";
}

} // namespace aa
