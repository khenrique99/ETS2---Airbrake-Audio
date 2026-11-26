// Airbrake Audio - app externo.
//
// Janela com as metricas ao vivo (pedal, velocidade, tempo de freada, som tocando) e os ajustes
// do plugin em controles deslizantes. Le status.json (escrito pelo plugin dentro do jogo) e manda
// comandos em command.txt; nao toca no processo do jogo. Sem o jogo aberto, o modo demonstracao
// simula freadas. Mesma identidade visual do Trailer Control (ui.h).
//
// Uso: AirbrakeAudio.exe [--demo]

#include "model.h"
#include "settings.h"
#include "shared/hotkey_combo.h"
#include "shared/ipc.h"
#include "texts.h"
#include "ui.h"
#include "version.h"

#include <windows.h>
#include <windowsx.h>
#include <dwmapi.h>
#include <shellapi.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <map>
#include <memory>
#include <string>
#include <vector>

using namespace aa::app;
using namespace aa::app::ui;
using Gdiplus::RectF;

namespace {

enum HitId {
    kHitNone = 0,
    kHitTopmost = 1,
    kHitFolder,
    kHitDemo,
    kHitSettings,
    kHitLang,
    kHitSave,
    kHitDefaults,
    kHitTab = 100,    // + Tab
    kHitSlider = 200, // + indice em fieldMetas
};

constexpr unsigned kTimerPoll = 1, kTimerSend = 2;
constexpr unsigned kPollMs = 100;
constexpr unsigned kSendMs = 90;           // envio enquanto arrasta
constexpr unsigned long long kReplyMs = 4000;
constexpr unsigned long long kOverrideMs = 1500; // valor local vale ate o plugin confirmar

// ------------------------------------------------------------------ estado do app

struct LocalValue {
    double value;
    unsigned long long until;
};

struct App {
    HWND hwnd = nullptr;
    float scale = 1.0f;
    std::unique_ptr<Source> source;
    AppState state;
    AppPrefs prefs;
    HWND gameHwnd = nullptr; // janela do jogo (vem do plugin) para "voltar ao jogo"

    std::vector<Hit> hits;
    int hover = kHitNone;
    int pressed = kHitNone;
    bool tracking = false;

    int dragging = -1;                         // indice do controle sendo arrastado
    std::map<std::string, LocalValue> local;   // valores mostrados antes do plugin confirmar
    std::map<std::string, double> toSend;      // ultimos valores ainda nao enviados

    long long pendingSeq = 0;
    unsigned long long pendingSince = 0;
    long long shownSeq = 0;
    std::wstring localMessage;
    bool localMessageOk = true;
};
App g;

float S(float v) { return v * g.scale; }
const wchar_t *tx(Txt id) { return T(g.prefs.lang, id); }
Tab currentTab() { return static_cast<Tab>(g.prefs.tab); }

std::string appHotkey() {
    const std::string h = aa::ipc::narrow(g.state.appHotkey);
    return h.empty() ? std::string(aa::hotkey::kDefault) : h;
}

bool isAppHotkey(WPARAM vk) {
    aa::hotkey::Combo c;
    if (!aa::hotkey::parse(appHotkey(), c)) return false;
    const bool numpadTwin = (c.vk == VK_OEM_MINUS && vk == VK_SUBTRACT) || (c.vk == VK_OEM_PLUS && vk == VK_ADD);
    return (c.vk == vk || numpadTwin) && c.ctrl == ((GetKeyState(VK_CONTROL) & 0x8000) != 0) &&
           c.shift == ((GetKeyState(VK_SHIFT) & 0x8000) != 0) && c.alt == ((GetKeyState(VK_MENU) & 0x8000) != 0);
}

bool live() { return g.source->isDemo() || (g.state.fileFound && g.state.connected); }

// ------------------------------------------------------------------ comandos

void showLocalMessage(Txt id, bool ok) {
    g.localMessage = tx(id);
    g.localMessageOk = ok;
}

void sendCommand(const std::string &action) {
    const long long seq = g.source->send(action);
    if (seq == 0) {
        showLocalMessage(Txt::CantWrite, false);
        return;
    }
    g.pendingSeq = seq;
    g.pendingSince = GetTickCount64();
    g.localMessage.clear();
}

std::string numberText(double v) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.6g", v);
    return buf;
}

void flushPendingValues() {
    for (const auto &kv : g.toSend) sendCommand("definir " + kv.first + " " + numberText(kv.second));
    g.toSend.clear();
}

// ------------------------------------------------------------------ valores dos controles

double shownValue(const FieldView &f) {
    const auto it = g.local.find(f.id);
    return it != g.local.end() ? it->second.value : f.value;
}

double snap(const FieldMeta &m, const FieldView &f, double display) {
    const double lo = f.min * m.scale, hi = f.max * m.scale;
    display = std::round(display / m.step) * m.step;
    return std::min(hi, std::max(lo, display)) / m.scale;
}

void setValue(const FieldMeta &m, const FieldView &f, double value, bool sendNow) {
    if (std::fabs(value - shownValue(f)) < 1e-9 && !sendNow) return;
    g.local[f.id] = {value, GetTickCount64() + kOverrideMs};
    // O ganho recalcula os sons (e silencia): so manda quando soltar o controle.
    const bool deferred = std::string(m.id) == "output.boost_db" && !sendNow;
    g.toSend[f.id] = value;
    if (sendNow) flushPendingValues();
    else if (!deferred) SetTimer(g.hwnd, kTimerSend, kSendMs, nullptr);
}

std::wstring valueText(const FieldMeta &m, double value) {
    wchar_t buf[48];
    swprintf_s(buf, L"%.*f %s", m.decimals, value * m.scale, m.unit);
    return buf;
}

// ------------------------------------------------------------------ janela

void returnToGame() {
    flushPendingValues();
    if (g.gameHwnd && IsWindow(g.gameHwnd)) SetForegroundWindow(g.gameHwnd);
    ShowWindow(g.hwnd, SW_MINIMIZE);
}

void showFromGame(HWND game) {
    if (game) g.gameHwnd = game;
    ShowWindow(g.hwnd, IsIconic(g.hwnd) ? SW_RESTORE : SW_SHOW);
    SetForegroundWindow(g.hwnd);
    InvalidateRect(g.hwnd, nullptr, FALSE);
}

void setTopmost(bool on) {
    g.prefs.topmost = on;
    g.prefs.save();
    SetWindowPos(g.hwnd, on ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

void setDemo(bool on) {
    if (on) g.source = std::make_unique<DemoSource>();
    else g.source = std::make_unique<PluginSource>();
    g.state = AppState{};
    g.local.clear();
    g.toSend.clear();
    g.pendingSeq = 0;
    g.localMessage.clear();
    if (on) showLocalMessage(Txt::DemoMessage, true);
    g.source->poll(g.state);
}

void openSettings() {
    const std::wstring path = !g.state.configPath.empty() ? g.state.configPath : g.prefs.lastConfigPath;
    if (!showSettingsDialog(g.hwnd, path, g.prefs.lang)) return;
    if (g.state.connected && !g.source->isDemo()) {
        sendCommand("recarregar_atalho");
        showLocalMessage(Txt::HotkeySaved, true);
    } else {
        showLocalMessage(Txt::HotkeySavedOffline, true);
    }
}

// ------------------------------------------------------------------ pintura

void paintHeader(Painter &p, float w) {
    Gdiplus::SolidBrush hb(P.header);
    p.graphics().FillRectangle(&hb, 0.f, 0.f, w, S(56));
    p.text(L"Airbrake Audio", RectF(S(20), 0, S(200), S(56)), S(18), P.headerText, true);
    if (!g.source->isDemo())
        p.text(std::wstring(tx(Txt::ReturnToGame)) + aa::hotkey::pretty(appHotkey()),
               RectF(S(186), 0, w - S(410), S(56)), S(12.5f), P.gray);

    Txt label;
    Color fg, bg;
    if (g.source->isDemo()) {
        label = Txt::ChipDemo;
        fg = P.accent;
        bg = P.selected;
    } else if (!g.state.fileFound || !g.state.connected) {
        label = Txt::ChipWaiting;
        fg = P.gray;
        bg = P.graySoft;
    } else if (g.state.paused) {
        label = Txt::ChipPaused;
        fg = P.amber;
        bg = P.amberSoft;
    } else {
        label = Txt::ChipConnected;
        fg = P.green;
        bg = P.greenSoft;
    }
    const float cw = S(196);
    p.chip(RectF(w - cw - S(20), S(14), cw, S(28)), tx(label), fg, bg);
}

void paintWaiting(Painter &p, const RectF &area) {
    p.card(area);
    const float cx = area.X + S(28);
    float y = area.Y + S(28);
    p.text(tx(Txt::WaitingTitle), RectF(cx, y, area.Width - S(56), S(30)), S(20), P.text, true);
    y += S(42);
    for (Txt line : {Txt::WaitingStep1, Txt::WaitingStep2, Txt::WaitingStep3}) {
        p.text(tx(line), RectF(cx, y, area.Width - S(56), S(24)), S(13.5f), P.muted);
        y += S(28);
    }
    if (g.state.fileFound) {
        y += S(4);
        p.text(tx(Txt::WaitingStale), RectF(cx, y, area.Width - S(56), S(24)), S(12.5f), P.amber);
        y += S(28);
    }
    y += S(10);
    p.button(RectF(cx, y, S(200), S(38)), tx(Txt::SeeDemo), kHitDemo, true, BtnStyle::Primary);
    p.button(RectF(cx + S(212), y, S(150), S(38)), tx(Txt::OpenFolder), kHitFolder, true, BtnStyle::Secondary);
}

void paintMetric(Painter &p, const RectF &r, Txt label, const std::wstring &value, const Color &valueColor,
                 double bar = -1) {
    p.fillRound(r, S(8), P.bg);
    p.text(tx(label), RectF(r.X + S(14), r.Y + S(8), r.Width - S(28), S(18)), S(12), P.muted);
    p.text(value, RectF(r.X + S(14), r.Y + S(28), r.Width - S(28), S(30)), S(22), valueColor, true);
    if (bar >= 0) {
        const RectF rail(r.X + S(14), r.Y + r.Height - S(14), r.Width - S(28), S(5));
        p.fillRound(rail, S(2.5f), P.graySoft);
        const float fill = rail.Width * static_cast<float>(std::min(1.0, std::max(0.0, bar)));
        if (fill > 0) p.fillRound(RectF(rail.X, rail.Y, fill, rail.Height), S(2.5f), P.accent);
    }
}

void paintMetrics(Painter &p, const RectF &card) {
    p.card(card);
    const AppState &s = g.state;
    const float gap = S(12);
    const RectF inner(card.X + S(12), card.Y + S(12), card.Width - S(24), card.Height - S(24));
    const float w = (inner.Width - gap * 3) / 4;
    wchar_t buf[32];

    swprintf_s(buf, L"%d%%", static_cast<int>(std::lround(s.pedal * 100)));
    paintMetric(p, RectF(inner.X, inner.Y, w, inner.Height), Txt::Pedal, buf, P.text, s.pedal);
    swprintf_s(buf, L"%d km/h", static_cast<int>(std::lround(s.speedKmh)));
    paintMetric(p, RectF(inner.X + (w + gap), inner.Y, w, inner.Height), Txt::Speed, buf, P.text);
    swprintf_s(buf, L"%.1f s", s.heldS);
    paintMetric(p, RectF(inner.X + (w + gap) * 2, inner.Y, w, inner.Height), Txt::BrakingFor, buf, P.text);

    Txt sound = Txt::SoundNone;
    Color color = P.muted;
    if (s.sound == "freio") {
        sound = Txt::SoundBrake;
        color = P.red;
    } else if (s.sound == "freio_alto") {
        sound = Txt::SoundBrakeHigh;
        color = P.red;
    } else if (s.sound == "ar") {
        sound = Txt::SoundAir;
        color = P.accent;
    }
    paintMetric(p, RectF(inner.X + (w + gap) * 3, inner.Y, w, inner.Height), Txt::Playing, tx(sound), color);
}

void paintSettings(Painter &p, const RectF &card) {
    p.card(card);
    const float top = card.Y + S(12);
    p.text(tx(Txt::Settings), RectF(card.X + S(20), top, S(110), S(32)), S(14), P.text, true);

    float x = card.X + S(130);
    for (Tab t : {Tab::Brake, Tab::Air, Tab::General}) {
        const Txt label = t == Tab::Brake ? Txt::TabBrake : t == Tab::Air ? Txt::TabAir : Txt::TabGeneral;
        x += p.tab(RectF(x, top, 0, S(32)), tx(label), kHitTab + static_cast<int>(t), t == currentTab()) + S(4);
    }

    const bool canEdit = live() && !g.state.fields.empty();
    const float right = card.X + card.Width - S(16);
    p.button(RectF(right - S(110), top, S(110), S(32)), tx(Txt::Save), kHitSave, canEdit && g.state.unsaved,
             BtnStyle::Primary);
    p.button(RectF(right - S(110) - S(10) - S(160), top, S(160), S(32)), tx(Txt::RestoreDefaults), kHitDefaults,
             canEdit, BtnStyle::Secondary);

    size_t count = 0;
    const FieldMeta *metas = fieldMetas(count);
    float y = top + S(48);
    const float rowH = S(42);
    const float labelW = std::min(S(290), card.Width * 0.38f);
    const float valueW = S(96);
    for (size_t i = 0; i < count; ++i) {
        const FieldMeta &m = metas[i];
        if (m.tab != currentTab()) continue;
        const FieldView *f = g.state.field(m.id);
        if (!f) continue;
        if (y + rowH > card.Y + card.Height) break;

        const int id = kHitSlider + static_cast<int>(i);
        const bool active = g.dragging == static_cast<int>(i);
        if (active || g.hover == id) p.fillRound(RectF(card.X + S(8), y, card.Width - S(16), rowH - S(2)), S(8), P.bg);

        const double value = shownValue(*f);
        p.text(g.prefs.lang == Lang::Pt ? m.pt : m.en, RectF(card.X + S(20), y, labelW, rowH), S(13.5f), P.text);
        const RectF track(card.X + S(20) + labelW + S(12), y, card.Width - labelW - valueW - S(64), rowH);
        p.slider(track, static_cast<float>((value - f->min) / (f->max - f->min)), id, active);

        const bool changed = std::fabs(value - f->def) > 1e-6;
        p.text(valueText(m, value), RectF(card.X + card.Width - valueW - S(20), y, valueW, rowH), S(13.5f),
               changed ? P.accent : P.muted, true, Align::Right);
        y += rowH;
    }
}

void paintMessage(Painter &p, const RectF &r) {
    std::wstring msg = g.localMessage;
    bool ok = g.localMessageOk;
    if (!g.state.soundsLoaded) {
        msg = std::wstring(tx(Txt::NoSound)) + g.state.soundError;
        ok = false;
    } else if (msg.empty()) {
        msg = g.state.lastCommandResult;
        ok = g.state.lastCommandOk;
    }
    if (msg.empty()) return;
    p.fillRound(r, S(8), ok ? P.greenSoft : P.redSoft);
    p.text(msg, RectF(r.X + S(14), r.Y, r.Width - S(28), r.Height), S(13), ok ? P.green : P.red, true);
}

void paintFooter(Painter &p, const RectF &r) {
    float x = r.X + r.Width;
    const float h = S(34), y = r.Y + (r.Height - h) / 2;
    x -= S(56);
    p.button(RectF(x, y, S(56), h), g.prefs.lang == Lang::Pt ? L"EN" : L"PT", kHitLang, true, BtnStyle::Secondary);
    x -= S(170);
    p.button(RectF(x, y, S(160), h), g.prefs.topmost ? L"✓ " + std::wstring(tx(Txt::AlwaysOnTop)) : tx(Txt::AlwaysOnTop),
             kHitTopmost, true, BtnStyle::Secondary);
    x -= S(130);
    p.button(RectF(x, y, S(120), h), tx(Txt::Configure), kHitSettings, true, BtnStyle::Secondary);
    x -= S(150);
    p.button(RectF(x, y, S(140), h), g.source->isDemo() ? tx(Txt::ExitDemo) : tx(Txt::Demo), kHitDemo, true,
             BtnStyle::Secondary);

    if (!live()) return;
    std::wstring left = L"plugin " + (g.state.pluginVersion.empty() ? L"?" : g.state.pluginVersion) + L"  ·  ";
    left += g.state.unsaved ? tx(Txt::Unsaved) : tx(Txt::AllSaved);
    p.text(left, RectF(r.X, r.Y, x - r.X - S(12), r.Height), S(12.5f), g.state.unsaved ? P.amber : P.muted,
           g.state.unsaved);
}

void paint(HDC hdc, const RECT &rc) {
    const float w = static_cast<float>(rc.right - rc.left), h = static_cast<float>(rc.bottom - rc.top);
    Gdiplus::Graphics gr(hdc);
    gr.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    gr.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);
    gr.Clear(P.bg);
    g.hits.clear();
    Painter p(gr, g.scale, g.hover, g.pressed, g.hits);

    paintHeader(p, w);
    const float m = S(20);
    const float footerH = S(52);
    const RectF footer(m, h - footerH, w - 2 * m, footerH);
    const RectF content(m, S(56) + m, w - 2 * m, h - S(56) - m - footerH - S(6));

    if (!live()) {
        paintWaiting(p, content);
    } else {
        const RectF metrics(content.X, content.Y, content.Width, S(100));
        paintMetrics(p, metrics);
        const float msgH = S(40);
        const RectF settings(content.X, metrics.Y + metrics.Height + S(14), content.Width,
                             content.Height - metrics.Height - S(14) - msgH - S(12));
        paintSettings(p, settings);
        paintMessage(p, RectF(content.X, settings.Y + settings.Height + S(12), content.Width, msgH));
    }
    paintFooter(p, footer);
}

// ------------------------------------------------------------------ entrada

const FieldMeta *sliderMeta(int index) {
    size_t count = 0;
    const FieldMeta *metas = fieldMetas(count);
    return index >= 0 && static_cast<size_t>(index) < count ? &metas[index] : nullptr;
}

void dragTo(int index, int mouseX, bool release) {
    const FieldMeta *m = sliderMeta(index);
    const Hit *hit = findHit(g.hits, kHitSlider + index);
    const FieldView *f = m ? g.state.field(m->id) : nullptr;
    if (!m || !hit || !f) return;
    // A area clicavel tem 10 px de folga de cada lado do trilho.
    const float x0 = hit->r.X + S(10), width = hit->r.Width - S(20);
    const double fraction = std::min(1.0, std::max(0.0, static_cast<double>((static_cast<float>(mouseX) - x0) / width)));
    const double display = (f->min + (f->max - f->min) * fraction) * m->scale;
    setValue(*m, *f, snap(*m, *f, display), release);
}

void nudge(int index, int steps) {
    const FieldMeta *m = sliderMeta(index);
    const FieldView *f = m ? g.state.field(m->id) : nullptr;
    if (!m || !f) return;
    setValue(*m, *f, snap(*m, *f, shownValue(*f) * m->scale + steps * m->step), true);
}

void onClick(int id) {
    if (id == kHitTopmost) setTopmost(!g.prefs.topmost);
    else if (id == kHitFolder) {
        const std::wstring dir = aa::ipc::folder();
        if (!dir.empty()) ShellExecuteW(g.hwnd, L"open", dir.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    } else if (id == kHitDemo) setDemo(!g.source->isDemo());
    else if (id == kHitSettings) openSettings();
    else if (id == kHitLang) {
        g.prefs.lang = g.prefs.lang == Lang::Pt ? Lang::En : Lang::Pt;
        g.prefs.save();
        if (g.source->isDemo()) showLocalMessage(Txt::DemoMessage, true);
    } else if (id == kHitSave) {
        flushPendingValues();
        sendCommand("salvar");
    } else if (id == kHitDefaults) {
        g.local.clear();
        g.toSend.clear();
        sendCommand("padrao");
    } else if (id >= kHitTab && id < kHitSlider) {
        g.prefs.tab = id - kHitTab;
        g.prefs.save();
    }
    InvalidateRect(g.hwnd, nullptr, FALSE);
}

void onPollTimer() {
    bool changed = g.source->poll(g.state);
    if (!g.state.configPath.empty() && g.state.configPath != g.prefs.lastConfigPath) {
        g.prefs.lastConfigPath = g.state.configPath; // lembra para configurar com o jogo fechado
        g.prefs.save();
    }

    const unsigned long long now = GetTickCount64();
    // Confirmacao: o plugin devolve o seq do ultimo comando processado.
    if (g.pendingSeq && g.state.lastCommandSeq >= g.pendingSeq) {
        g.pendingSeq = 0;
    } else if (g.pendingSeq && now - g.pendingSince > kReplyMs) {
        g.pendingSeq = 0;
        showLocalMessage(Txt::NoReply, false);
        changed = true;
    }
    if (g.state.lastCommandSeq != g.shownSeq) {
        g.shownSeq = g.state.lastCommandSeq;
        g.localMessage.clear();
    }
    // Valores locais saem quando o plugin confirmou (ou desistiu).
    for (auto it = g.local.begin(); it != g.local.end();) {
        const FieldView *f = g.state.field(it->first);
        const bool dragging = g.dragging >= 0 && sliderMeta(g.dragging) && it->first == sliderMeta(g.dragging)->id;
        const bool confirmed = f && std::fabs(f->value - it->second.value) < 1e-4;
        if (!dragging && !g.toSend.count(it->first) && (confirmed || now > it->second.until)) it = g.local.erase(it);
        else ++it;
    }
    if (changed) InvalidateRect(g.hwnd, nullptr, FALSE);
}

void updateScale(HWND hwnd) { g.scale = static_cast<float>(GetDpiForWindow(hwnd)) / 96.0f; }

void paintBuffered(HWND hwnd) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    RECT rc;
    GetClientRect(hwnd, &rc);
    HDC mem = CreateCompatibleDC(hdc);
    HBITMAP bmp = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
    HGDIOBJ old = SelectObject(mem, bmp);
    paint(mem, rc);
    BitBlt(hdc, 0, 0, rc.right, rc.bottom, mem, 0, 0, SRCCOPY);
    SelectObject(mem, old);
    DeleteObject(bmp);
    DeleteDC(mem);
    EndPaint(hwnd, &ps);
}

LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    // Com a janela de Configuracoes aberta a principal fica desabilitada: ignora entrada.
    const bool input = msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP || msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN ||
                       msg == WM_MOUSEWHEEL;
    if (input && !IsWindowEnabled(hwnd)) return 0;
    switch (msg) {
    case WM_CREATE:
        g.hwnd = hwnd;
        updateScale(hwnd);
        SetTimer(hwnd, kTimerPoll, kPollMs, nullptr);
        return 0;
    case WM_TIMER:
        if (wp == kTimerSend) {
            KillTimer(hwnd, kTimerSend);
            flushPendingValues();
        } else {
            onPollTimer();
        }
        return 0;
    case WM_DPICHANGED: {
        updateScale(hwnd);
        const RECT *r = reinterpret_cast<const RECT *>(lp);
        SetWindowPos(hwnd, nullptr, r->left, r->top, r->right - r->left, r->bottom - r->top,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        return 0;
    }
    case WM_GETMINMAXINFO: {
        auto *mmi = reinterpret_cast<MINMAXINFO *>(lp);
        mmi->ptMinTrackSize.x = static_cast<LONG>(S(820));
        mmi->ptMinTrackSize.y = static_cast<LONG>(S(640));
        return 0;
    }
    case WM_SIZE: InvalidateRect(hwnd, nullptr, FALSE); return 0;
    case WM_ERASEBKGND: return 1;
    case WM_PAINT: paintBuffered(hwnd); return 0;
    case WM_MOUSEMOVE: {
        if (!g.tracking) {
            TRACKMOUSEEVENT tme{sizeof(tme), TME_LEAVE, hwnd, 0};
            TrackMouseEvent(&tme);
            g.tracking = true;
        }
        if (g.dragging >= 0) {
            dragTo(g.dragging, GET_X_LPARAM(lp), false);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        const int h = hitTest(g.hits, GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
        if (h != g.hover) {
            g.hover = h;
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        SetCursor(LoadCursor(nullptr, h != kHitNone ? IDC_HAND : IDC_ARROW));
        return 0;
    }
    case WM_MOUSELEAVE:
        g.tracking = false;
        if (g.dragging < 0) {
            g.hover = kHitNone;
            g.pressed = kHitNone;
        }
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_SETCURSOR:
        if (LOWORD(lp) == HTCLIENT) return TRUE; // cursor definido em WM_MOUSEMOVE
        break;
    case WM_LBUTTONDOWN: {
        const int h = hitTest(g.hits, GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
        SetCapture(hwnd);
        if (h >= kHitSlider) {
            g.dragging = h - kHitSlider;
            dragTo(g.dragging, GET_X_LPARAM(lp), false);
        } else {
            g.pressed = h;
        }
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }
    case WM_LBUTTONUP: {
        ReleaseCapture();
        if (g.dragging >= 0) {
            dragTo(g.dragging, GET_X_LPARAM(lp), true);
            g.dragging = -1;
        } else {
            const int h = hitTest(g.hits, GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
            const int pressed = g.pressed;
            g.pressed = kHitNone;
            if (h != kHitNone && h == pressed) onClick(h);
        }
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }
    case WM_MOUSEWHEEL: {
        POINT pt{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
        ScreenToClient(hwnd, &pt);
        const int h = hitTest(g.hits, pt.x, pt.y);
        if (h >= kHitSlider) {
            nudge(h - kHitSlider, GET_WHEEL_DELTA_WPARAM(wp) > 0 ? 1 : -1);
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;
    }
    case WM_SYSKEYDOWN:
        if (isAppHotkey(wp)) {
            returnToGame();
            return 0;
        }
        break;
    case WM_KEYDOWN:
        if (wp == VK_ESCAPE || isAppHotkey(wp)) returnToGame();
        else if (wp == 'S' && (GetKeyState(VK_CONTROL) & 0x8000) && live()) onClick(kHitSave);
        return 0;
    case aa::ipc::kMsgShowFromGame: showFromGame(reinterpret_cast<HWND>(wp)); return 0;
    case WM_DESTROY:
        flushPendingValues();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

} // namespace

int WINAPI wWinMain(HINSTANCE inst, HINSTANCE, PWSTR cmdLine, int show) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    // So uma janela: se ja houver outra, traz para frente.
    HANDLE mutex = CreateMutexW(nullptr, TRUE, L"Local\\ETS2AirbrakeAudioApp");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        if (HWND other = FindWindowW(aa::ipc::kAppWindowClass, nullptr)) {
            ShowWindow(other, SW_RESTORE);
            SetForegroundWindow(other);
        }
        return 0;
    }

    Gdiplus::GdiplusStartupInput gdiIn;
    ULONG_PTR gdiToken = 0;
    Gdiplus::GdiplusStartup(&gdiToken, &gdiIn, nullptr);

    const bool demo = cmdLine && wcsstr(cmdLine, L"--demo");
    // Aberto pelo plugin (atalho no jogo): "--from-game <hwnd do jogo>"
    if (const wchar_t *fg = cmdLine ? wcsstr(cmdLine, L"--from-game") : nullptr)
        g.gameHwnd = reinterpret_cast<HWND>(static_cast<uintptr_t>(_wcstoui64(fg + 11, nullptr, 10)));
    g.prefs.load();
    setDemo(demo);

    WNDCLASSEXW wc{sizeof(wc)};
    wc.lpfnWndProc = wndProc;
    wc.hInstance = inst;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon = LoadIconW(inst, MAKEINTRESOURCEW(1));
    wc.hIconSm = wc.hIcon;
    wc.lpszClassName = aa::ipc::kAppWindowClass;
    RegisterClassExW(&wc);

    const UINT dpi = GetDpiForSystem();
    const int w = MulDiv(900, static_cast<int>(dpi), 96), h = MulDiv(680, static_cast<int>(dpi), 96);
    HWND hwnd = CreateWindowExW(0, wc.lpszClassName, L"Airbrake Audio " AIRBRAKE_AUDIO_VERSION_W, WS_OVERLAPPEDWINDOW,
                                CW_USEDEFAULT, CW_USEDEFAULT, w, h, nullptr, nullptr, inst, nullptr);
    if (!hwnd) return 1;

    // Barra de titulo escura, combinando com o cabecalho.
    BOOL dark = TRUE;
    DwmSetWindowAttribute(hwnd, 20 /* DWMWA_USE_IMMERSIVE_DARK_MODE */, &dark, sizeof(dark));
    if (g.prefs.topmost) SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    ShowWindow(hwnd, show);
    UpdateWindow(hwnd);

    MSG m;
    while (GetMessageW(&m, nullptr, 0, 0)) {
        TranslateMessage(&m);
        DispatchMessageW(&m);
    }
    g.source.reset();
    Gdiplus::GdiplusShutdown(gdiToken);
    if (mutex) CloseHandle(mutex);
    return static_cast<int>(m.wParam);
}
