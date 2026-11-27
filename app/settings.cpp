#include "settings.h"

#include "shared/hotkey_combo.h"
#include "shared/ini_edit.h"
#include "shared/ipc.h"

#include <cctype>

namespace aa::app {

// ------------------------------------------------------------------ janela: so o atalho
//
// Mostra o atalho atual; "Alterar" entra no modo de captura: a proxima combinacao apertada
// vira o atalho (Esc cancela). Salvar grava [ui] hotkey no airbrake_audio.ini.

namespace {

enum Ctl {
    kLblCombo = 101,
    kLblHint,
    kBtnChange,
    kBtnDefault,
    kBtnSave,
    kBtnCancel,
};

struct Dialog {
    HWND hwnd = nullptr;
    HFONT font = nullptr, bold = nullptr, big = nullptr;
    float scale = 1.0f;
    Lang lang = Lang::Pt;
    std::wstring configPath;
    std::string iniText;
    std::string combo; // atalho escolhido (formato do .ini)
    bool capturing = false;
    bool saved = false;
    bool done = false;
};
Dialog *g_dlg = nullptr;

const wchar_t *tx(Txt id) { return T(g_dlg->lang, id); }

int Sx(int v) { return static_cast<int>(static_cast<float>(v) * g_dlg->scale + 0.5f); }

HWND add(const wchar_t *cls, const wchar_t *text, DWORD style, int x, int y, int w, int h, int id, HFONT font) {
    HWND c = CreateWindowExW(0, cls, text, WS_CHILD | WS_VISIBLE | style, Sx(x), Sx(y), Sx(w), Sx(h), g_dlg->hwnd,
                             reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), nullptr, nullptr);
    SendMessageW(c, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    return c;
}

HWND item(int id) { return GetDlgItem(g_dlg->hwnd, id); }

void refresh() {
    SetWindowTextW(item(kLblCombo), g_dlg->capturing ? tx(Txt::DlgPress) : hotkey::pretty(g_dlg->combo).c_str());
    SetWindowTextW(item(kLblHint), g_dlg->capturing ? tx(Txt::DlgHintCapture) : tx(Txt::DlgHintIdle));
    SetWindowTextW(item(kBtnChange), g_dlg->capturing ? tx(Txt::DlgCancelCapture) : tx(Txt::DlgChange));
    EnableWindow(item(kBtnSave), !g_dlg->capturing && !g_dlg->iniText.empty());
    EnableWindow(item(kBtnDefault), !g_dlg->capturing && !g_dlg->iniText.empty());
}

void build() {
    add(L"STATIC", tx(Txt::DlgHeading), SS_LEFT, 20, 18, 420, 24, 0, g_dlg->bold);
    add(L"STATIC", L"", SS_CENTER | SS_CENTERIMAGE | SS_SUNKEN | SS_NOPREFIX, 20, 52, 420, 56, kLblCombo, g_dlg->big);
    add(L"STATIC", L"", SS_LEFT, 20, 116, 420, 22, kLblHint, g_dlg->font);
    add(L"BUTTON", tx(Txt::DlgChange), BS_PUSHBUTTON | WS_TABSTOP, 20, 146, 170, 34, kBtnChange, g_dlg->font);
    add(L"BUTTON", tx(Txt::DlgUseDefault), BS_PUSHBUTTON | WS_TABSTOP, 200, 146, 240, 34, kBtnDefault, g_dlg->font);
    if (g_dlg->iniText.empty()) {
        add(L"STATIC", tx(Txt::DlgNoIni), SS_LEFT, 20, 192, 420, 40, 0, g_dlg->font);
        EnableWindow(item(kBtnChange), FALSE);
    }
    add(L"BUTTON", tx(Txt::DlgCancel), BS_PUSHBUTTON | WS_TABSTOP, 230, 240, 100, 34, kBtnCancel, g_dlg->font);
    add(L"BUTTON", tx(Txt::DlgSave), BS_DEFPUSHBUTTON | WS_TABSTOP, 340, 240, 100, 34, kBtnSave, g_dlg->font);
    refresh();
}

void save() {
    const std::string updated = ini::apply(g_dlg->iniText, {{"ui", "hotkey", g_dlg->combo}});
    if (!ipc::writeAtomic(g_dlg->configPath, updated)) {
        MessageBoxW(g_dlg->hwnd, (g_dlg->configPath + L"\n\n" + tx(Txt::DlgWriteError)).c_str(), tx(Txt::DlgTitle),
                    MB_ICONERROR);
        return;
    }
    g_dlg->saved = true;
    g_dlg->done = true;
}

bool isModifier(unsigned vk) {
    return vk == VK_CONTROL || vk == VK_LCONTROL || vk == VK_RCONTROL || vk == VK_SHIFT || vk == VK_LSHIFT ||
           vk == VK_RSHIFT || vk == VK_MENU || vk == VK_LMENU || vk == VK_RMENU || vk == VK_LWIN || vk == VK_RWIN;
}

// Tecla apertada durante a captura.
void onCaptureKey(unsigned vk) {
    if (isModifier(vk)) return; // espera a tecla "de verdade"
    if (vk == VK_ESCAPE) {
        g_dlg->capturing = false;
        refresh();
        return;
    }
    const bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    const bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    const bool alt = (GetKeyState(VK_MENU) & 0x8000) != 0;
    const std::string combo = hotkey::makeCombo(ctrl, shift, alt, vk);
    hotkey::Combo parsed;
    if (combo.empty() || !hotkey::parse(combo, parsed)) {
        SetWindowTextW(item(kLblHint), tx(Txt::DlgInvalidKey));
        return;
    }
    if (!hotkey::safeForGame(parsed)) {
        SetWindowTextW(item(kLblHint), tx(Txt::DlgNeedModifier));
        return;
    }
    g_dlg->combo = combo;
    g_dlg->capturing = false;
    refresh();
    SetFocus(item(kBtnSave));
}

LRESULT CALLBACK proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (!g_dlg || g_dlg->hwnd != hwnd) return DefWindowProcW(hwnd, msg, wp, lp);
    switch (msg) {
    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case kBtnSave: save(); return 0;
        case IDCANCEL:
        case kBtnCancel: g_dlg->done = true; return 0;
        case kBtnChange:
            g_dlg->capturing = !g_dlg->capturing;
            refresh();
            SetFocus(hwnd); // as teclas vao para a janela, nao para um botao
            return 0;
        case kBtnDefault:
            g_dlg->combo = hotkey::kDefault;
            refresh();
            return 0;
        }
        break;
    case WM_CTLCOLORSTATIC: {
        HDC dc = reinterpret_cast<HDC>(wp);
        SetBkColor(dc, GetSysColor(COLOR_WINDOW));
        if (reinterpret_cast<HWND>(lp) == item(kLblCombo))
            SetTextColor(dc, g_dlg->capturing ? RGB(154, 103, 0) : RGB(37, 99, 235));
        return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_WINDOW));
    }
    case WM_CLOSE: g_dlg->done = true; return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

} // namespace

bool showSettingsDialog(HWND owner, const std::wstring &configPath, Lang lang) {
    if (g_dlg) { // ja tem uma aberta: so traz para frente
        SetForegroundWindow(g_dlg->hwnd);
        return false;
    }
    Dialog dlg;
    g_dlg = &dlg;
    dlg.lang = lang;
    dlg.configPath = configPath;
    dlg.scale = static_cast<float>(GetDpiForWindow(owner)) / 96.0f;
    if (!configPath.empty()) ipc::readAll(configPath, dlg.iniText);
    hotkey::Combo current;
    dlg.combo = ini::get(dlg.iniText, "ui", "hotkey");
    if (!hotkey::parse(dlg.combo, current)) dlg.combo = hotkey::kDefault;

    static bool registered = false;
    if (!registered) {
        WNDCLASSEXW wc{sizeof(wc)};
        wc.lpfnWndProc = proc;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = GetSysColorBrush(COLOR_WINDOW);
        wc.lpszClassName = L"ETS2AirbrakeAudioSettings";
        RegisterClassExW(&wc);
        registered = true;
    }

    auto mkFont = [](int px, int weight) {
        return CreateFontW(-Sx(px), 0, 0, 0, weight, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
    };
    dlg.font = mkFont(14, FW_NORMAL);
    dlg.bold = mkFont(15, FW_SEMIBOLD);
    dlg.big = mkFont(24, FW_SEMIBOLD);

    const UINT dpi = GetDpiForWindow(owner);
    RECT rc{0, 0, Sx(460), Sx(292)};
    AdjustWindowRectExForDpi(&rc, WS_CAPTION | WS_SYSMENU, FALSE, WS_EX_DLGMODALFRAME, dpi);
    RECT orc;
    GetWindowRect(owner, &orc);
    const int w = rc.right - rc.left, h = rc.bottom - rc.top;
    dlg.hwnd = CreateWindowExW(WS_EX_DLGMODALFRAME, L"ETS2AirbrakeAudioSettings", tx(Txt::DlgTitle),
                               WS_CAPTION | WS_SYSMENU, orc.left + ((orc.right - orc.left) - w) / 2,
                               orc.top + ((orc.bottom - orc.top) - h) / 2, w, h, owner, nullptr, nullptr, nullptr);
    build();
    EnableWindow(owner, FALSE);
    ShowWindow(dlg.hwnd, SW_SHOW);

    MSG m{};
    bool quit = false;
    while (!dlg.done) {
        if (GetMessageW(&m, nullptr, 0, 0) <= 0) {
            quit = true;
            break;
        }
        const bool key = m.message == WM_KEYDOWN || m.message == WM_SYSKEYDOWN;
        if (key && dlg.capturing) {
            onCaptureKey(static_cast<unsigned>(m.wParam));
            continue;
        }
        if (key && m.wParam == VK_ESCAPE) {
            dlg.done = true;
            continue;
        }
        if (!IsDialogMessageW(dlg.hwnd, &m)) {
            TranslateMessage(&m);
            DispatchMessageW(&m);
        }
    }
    EnableWindow(owner, TRUE);
    SetForegroundWindow(owner);
    g_dlg = nullptr; // antes de destruir: mensagens da destruicao vao para DefWindowProc
    DestroyWindow(dlg.hwnd);
    DeleteObject(dlg.font);
    DeleteObject(dlg.bold);
    DeleteObject(dlg.big);
    if (quit) PostQuitMessage(static_cast<int>(m.wParam));
    return dlg.saved;
}

} // namespace aa::app
