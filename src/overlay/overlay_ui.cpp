#include "overlay_ui.h"

#include "shared/hotkey_combo.h"
#include "shared/ipc.h"
#include "texts.h"
#include "version.h"

#include <windows.h>

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <vector>

namespace aa::ui {

namespace {

using app::Lang;
using app::Tab;
using app::Txt;

// Paleta do app externo (app/ui.h), para o painel manter a mesma cara.
constexpr ImU32 rgb(int r, int g, int b, int a = 255) { return IM_COL32(r, g, b, a); }
const ImU32 kBg = rgb(243, 244, 246);
const ImU32 kHeader = rgb(24, 31, 42);
const ImU32 kWhite = rgb(255, 255, 255);
const ImU32 kBorder = rgb(222, 226, 231);
const ImU32 kText = rgb(31, 35, 40);
const ImU32 kMuted = rgb(101, 109, 118);
const ImU32 kAccent = rgb(37, 99, 235);
const ImU32 kAccentHover = rgb(29, 78, 216);
const ImU32 kGreen = rgb(26, 127, 55);
const ImU32 kGreenSoft = rgb(220, 245, 228);
const ImU32 kRed = rgb(207, 34, 46);
const ImU32 kRedDark = rgb(170, 25, 35);
const ImU32 kRedSoft = rgb(255, 230, 232);
const ImU32 kAmber = rgb(154, 103, 0);
const ImU32 kAmberSoft = rgb(255, 244, 214);
const ImU32 kGray = rgb(140, 149, 159);
const ImU32 kGraySoft = rgb(234, 238, 242);
const ImU32 kSelected = rgb(232, 240, 254);

constexpr unsigned long long kSendMs = 90;       // intervalo de envio enquanto arrasta
constexpr unsigned long long kOverrideMs = 1500; // valor local vale ate o plugin confirmar
constexpr unsigned long long kLocalMessageMs = 6000;

enum HitId {
    kHitSave = 1,
    kHitDefaults,
    kHitLang,
    kHitClose,
    kHitTab = 100,    // + Tab
    kHitSlider = 200, // + indice na linha
};

const Config &defaults() {
    static const Config d;
    return d;
}

// Texto da tela (UTF-16 em texts.cpp) em UTF-8 para o ImGui. Os ponteiros sao literais fixos.
const std::string &u8(const wchar_t *w) {
    static std::map<const wchar_t *, std::string> cache;
    auto it = cache.find(w);
    if (it == cache.end()) it = cache.emplace(w, ipc::narrow(w)).first;
    return it->second;
}

const std::string &tx(const Context &ctx, Txt id) { return u8(app::T(ctx.prefs.lang, id)); }

struct Painter {
    ImDrawList *dl;
    const Fonts &f;
    float s; // escala
    float S(float v) const { return v * s; }
};

struct Rect {
    float x, y, w, h;
    ImVec2 min() const { return {x, y}; }
    ImVec2 max() const { return {x + w, y + h}; }
};

enum class Align { Left, Center, Right };

float textW(ImFont *font, const std::string &t) {
    return font->CalcTextSizeA(font->FontSize, FLT_MAX, 0.0f, t.c_str()).x;
}

// Corta no limite da caixa com reticencias (a fonte pode nao ter "…": usa "...").
std::string fitText(ImFont *font, const std::string &t, float maxW, const char *ellipsis) {
    if (textW(font, t) <= maxW) return t;
    std::string out = t;
    while (!out.empty()) {
        do { // volta ate o inicio de um caractere UTF-8
            out.pop_back();
        } while (!out.empty() && (static_cast<unsigned char>(out.back()) & 0xC0) == 0x80);
        if (textW(font, out + ellipsis) <= maxW) return out + ellipsis;
    }
    return std::string();
}

void fillRound(const Painter &p, const Rect &r, float radius, ImU32 c, ImDrawFlags flags = 0) {
    p.dl->AddRectFilled(r.min(), r.max(), c, radius, flags);
}

void strokeRound(const Painter &p, const Rect &r, float radius, ImU32 c, float width = 1.0f) {
    p.dl->AddRect(r.min(), r.max(), c, radius, 0, width);
}

void text(const Painter &p, ImFont *font, const std::string &t, const Rect &r, ImU32 col, Align a = Align::Left,
          bool vcenter = true) {
    const std::string s = fitText(font, t, r.w, p.f.unicode ? "\xE2\x80\xA6" : "...");
    if (s.empty()) return;
    const ImVec2 sz = font->CalcTextSizeA(font->FontSize, FLT_MAX, 0.0f, s.c_str());
    float x = r.x;
    if (a == Align::Center) x = r.x + (r.w - sz.x) * 0.5f;
    else if (a == Align::Right) x = r.x + r.w - sz.x;
    const float y = vcenter ? r.y + (r.h - sz.y) * 0.5f : r.y;
    p.dl->AddText(font, font->FontSize, ImVec2(std::floor(x), std::floor(y)), col, s.c_str());
}

// Area clicavel invisivel; hover/held pelo ImGui.
struct HitState {
    bool clicked = false, hovered = false, held = false;
};

HitState hit(int id, const Rect &r, bool enabled = true, bool allowOverlap = false) {
    HitState h;
    if (!enabled) return h;
    ImGui::PushID(id);
    ImGui::SetCursorScreenPos(r.min());
    if (allowOverlap) ImGui::SetNextItemAllowOverlap();
    h.clicked = ImGui::InvisibleButton("##hit", ImVec2(r.w, r.h));
    h.hovered = ImGui::IsItemHovered();
    h.held = ImGui::IsItemActive();
    ImGui::PopID();
    return h;
}

enum class BtnStyle { Primary, Danger, Secondary };

bool button(const Painter &p, int id, const Rect &r, const std::string &label, bool enabled, BtnStyle style) {
    const HitState h = hit(id, r, enabled);
    ImU32 fill, fg, border = 0;
    if (!enabled) {
        fill = kGraySoft;
        fg = kGray;
        border = kGraySoft;
    } else if (style == BtnStyle::Primary) {
        fill = h.hovered ? kAccentHover : kAccent;
        fg = kWhite;
    } else if (style == BtnStyle::Danger) {
        fill = h.hovered ? kRedDark : kRed;
        fg = kWhite;
    } else {
        fill = h.hovered ? kGraySoft : kWhite;
        fg = kText;
        border = kBorder;
    }
    Rect rr = r;
    if (h.held && h.hovered) rr.y += p.S(1);
    fillRound(p, rr, p.S(6), fill);
    if (style == BtnStyle::Secondary || !enabled) strokeRound(p, rr, p.S(6), border);
    text(p, p.f.bold, label, rr, fg, Align::Center);
    return h.clicked;
}

void chip(const Painter &p, const Rect &r, const std::string &label, ImU32 fg, ImU32 bg) {
    fillRound(p, r, r.h * 0.5f, bg);
    const float d = p.S(8);
    p.dl->AddCircleFilled(ImVec2(r.x + p.S(10) + d * 0.5f, r.y + r.h * 0.5f), d * 0.5f, fg, 16);
    text(p, p.f.bold, label, Rect{r.x + p.S(22), r.y, r.w - p.S(26), r.h}, fg);
}

// Aba em "pilula"; devolve a largura usada e diz em *clicked se foi clicada.
float tabPill(const Painter &p, int id, const Rect &r, const std::string &label, bool active, bool *clicked) {
    const float w = textW(p.f.bold, label) + p.S(28);
    const Rect rr{r.x, r.y, w, r.h};
    const HitState h = hit(id, rr);
    if (active) fillRound(p, rr, rr.h * 0.5f, kSelected);
    else if (h.hovered) fillRound(p, rr, rr.h * 0.5f, kBg);
    text(p, p.f.bold, label, rr, active ? kAccent : kMuted, Align::Center);
    *clicked = h.clicked;
    return w;
}

// ------------------------------------------------------------------ ajustes

// Um ajuste mostrado na tela: como aparece (texts.cpp) + faixa e valor (config.cpp).
struct Row {
    const app::FieldMeta *meta;
    const FloatField *field;
};

std::vector<Row> rowsFor(Tab tab) {
    std::vector<Row> rows;
    size_t count = 0;
    const app::FieldMeta *metas = app::fieldMetas(count);
    for (size_t i = 0; i < count; ++i) {
        if (metas[i].tab != tab) continue;
        const FloatField *f = findField(metas[i].id);
        if (f && f->live) rows.push_back({&metas[i], f});
    }
    return rows;
}

double shownValue(const Context &ctx, const Config &cfg, const Row &r) {
    const auto it = ctx.local.find(r.meta->id);
    return it != ctx.local.end() ? it->second.value : static_cast<double>(cfg.*(r.field->member));
}

// Arredonda ao passo do controle e limita a faixa; entra e sai no valor do .ini (nao no mostrado).
double snap(const Row &r, double display) {
    const double lo = r.field->low * r.meta->scale, hi = r.field->high * r.meta->scale;
    display = std::round(display / r.meta->step) * r.meta->step;
    return std::min(hi, std::max(lo, display)) / r.meta->scale;
}

std::string numberText(double v) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.6g", v);
    return buf;
}

void setLocalMessage(Context &ctx, const std::string &m, bool ok) {
    ctx.localMessage = m;
    ctx.localOk = ok;
    ctx.localMessageAt = GetTickCount64();
}

void submit(PanelHost &host, Context &ctx, const std::string &action) {
    host.submit(action);
    ctx.messageArmed = true;
    ctx.localMessage.clear();
}

void sendPending(PanelHost &host, Context &ctx, bool all) {
    for (auto it = ctx.toSend.begin(); it != ctx.toSend.end();) {
        // O ganho recalcula os sons (e silencia): so vai quando soltar o controle.
        if (!all && it->first == ctx.dragging && it->first == "output.boost_db") {
            ++it;
            continue;
        }
        submit(host, ctx, "definir " + it->first + " " + numberText(it->second));
        it = ctx.toSend.erase(it);
    }
    ctx.lastSendMs = GetTickCount64();
}

void setValue(PanelHost &host, Context &ctx, const Config &cfg, const Row &r, double value, bool sendNow) {
    if (std::fabs(value - shownValue(ctx, cfg, r)) < 1e-9 && !sendNow) return;
    ctx.local[r.meta->id] = {value, GetTickCount64() + kOverrideMs};
    ctx.toSend[r.meta->id] = value;
    if (sendNow) sendPending(host, ctx, true);
}

void nudge(PanelHost &host, Context &ctx, const Config &cfg, const Row &r, int steps, bool sendNow) {
    setValue(host, ctx, cfg, r, snap(r, shownValue(ctx, cfg, r) * r.meta->scale + steps * r.meta->step), sendNow);
}

std::string valueText(const Row &r, double value) {
    char buf[48];
    std::snprintf(buf, sizeof(buf), "%.*f %s", r.meta->decimals, value * r.meta->scale, u8(r.meta->unit).c_str());
    return buf;
}

void setTab(Context &ctx, int tab) {
    ctx.prefs.tab = ((tab % 3) + 3) % 3;
    ctx.prefs.save();
    ctx.selected = 0;
    ctx.dragging.clear();
}

// Valores locais saem quando o plugin confirmou (ou desistiu).
void updateLocals(Context &ctx, const Config &cfg) {
    const unsigned long long now = GetTickCount64();
    for (auto it = ctx.local.begin(); it != ctx.local.end();) {
        const FloatField *f = findField(it->first);
        const bool confirmed = f && std::fabs(static_cast<double>(cfg.*(f->member)) - it->second.value) < 1e-4;
        const bool busy = it->first == ctx.dragging || ctx.toSend.count(it->first) > 0;
        if (!busy && (confirmed || now > it->second.until)) it = ctx.local.erase(it);
        else ++it;
    }
    if (!ctx.localMessage.empty() && now - ctx.localMessageAt > kLocalMessageMs) ctx.localMessage.clear();
}

// Teclado: setas escolhem a linha e ajustam, Tab troca a aba, Ctrl+S salva (alternativa ao mouse).
void handleKeys(PanelHost &host, Context &ctx, const PanelSnapshot &st) {
    const std::vector<Row> rows = rowsFor(static_cast<Tab>(ctx.prefs.tab));
    const int count = static_cast<int>(rows.size());
    const int nav = ctx.keyNav.exchange(0);
    if (nav != 0 && count > 0) {
        ctx.selected = std::clamp(ctx.selected + nav, 0, count - 1);
        ctx.selectionVisible = true;
    }
    ctx.selected = std::clamp(ctx.selected, 0, std::max(0, count - 1));

    const int adjust = ctx.keyAdjust.exchange(0);
    if (adjust != 0 && count > 0) {
        ctx.selectionVisible = true;
        nudge(host, ctx, st.config, rows[static_cast<size_t>(ctx.selected)], adjust, false);
    }

    const int tab = ctx.keyTab.exchange(0);
    if (tab != 0) setTab(ctx, ctx.prefs.tab + tab);

    if (ctx.keySave.exchange(false) && st.unsaved) {
        sendPending(host, ctx, true);
        submit(host, ctx, "salvar");
    }
}

// ------------------------------------------------------------------ secoes

void paintHeader(const Painter &p, const Context &ctx, const Rect &r, const PanelSnapshot &st) {
    fillRound(p, Rect{r.x, r.y, r.w, p.S(56)}, p.S(10), kHeader, ImDrawFlags_RoundCornersTop);
    text(p, p.f.title, "Airbrake Audio", Rect{r.x + p.S(20), r.y, p.S(180), p.S(56)}, kWhite);
    text(p, p.f.tiny, tx(ctx, Txt::ReturnToGame) + ipc::narrow(hotkey::pretty(st.config.hotkey)),
         Rect{r.x + p.S(196), r.y, r.w - p.S(430), p.S(56)}, kGray);
    const bool paused = st.paused;
    const float cw = p.S(196);
    chip(p, Rect{r.x + r.w - cw - p.S(20), r.y + p.S(14), cw, p.S(28)},
         tx(ctx, paused ? Txt::ChipPaused : Txt::ChipConnected), paused ? kAmber : kGreen,
         paused ? kAmberSoft : kGreenSoft);
}

void paintMetric(const Painter &p, const Context &ctx, const Rect &r, Txt label, const std::string &value,
                 ImU32 valueColor, double bar = -1) {
    fillRound(p, r, p.S(8), kBg);
    text(p, p.f.tiny, tx(ctx, label), Rect{r.x + p.S(14), r.y + p.S(8), r.w - p.S(28), p.S(18)}, kMuted);
    text(p, p.f.mid, value, Rect{r.x + p.S(14), r.y + p.S(28), r.w - p.S(28), p.S(30)}, valueColor);
    if (bar >= 0) {
        const Rect rail{r.x + p.S(14), r.y + r.h - p.S(14), r.w - p.S(28), p.S(5)};
        fillRound(p, rail, p.S(2.5f), kGraySoft);
        const float fill = rail.w * static_cast<float>(std::clamp(bar, 0.0, 1.0));
        if (fill > 0) fillRound(p, Rect{rail.x, rail.y, fill, rail.h}, p.S(2.5f), kAccent);
    }
}

void paintMetrics(const Painter &p, const Context &ctx, const Rect &card, const PanelSnapshot &st) {
    fillRound(p, card, p.S(10), kWhite);
    strokeRound(p, card, p.S(10), kBorder);
    const float gap = p.S(12);
    const Rect inner{card.x + p.S(12), card.y + p.S(12), card.w - p.S(24), card.h - p.S(24)};
    const float w = (inner.w - gap * 3) / 4;
    char buf[32];

    std::snprintf(buf, sizeof(buf), "%d%%", static_cast<int>(std::lround(st.pedal * 100)));
    paintMetric(p, ctx, Rect{inner.x, inner.y, w, inner.h}, Txt::Pedal, buf, kText, st.pedal);
    std::snprintf(buf, sizeof(buf), "%d km/h", static_cast<int>(std::lround(st.speedKmh)));
    paintMetric(p, ctx, Rect{inner.x + (w + gap), inner.y, w, inner.h}, Txt::Speed, buf, kText);
    std::snprintf(buf, sizeof(buf), "%.1f s", st.heldS);
    paintMetric(p, ctx, Rect{inner.x + (w + gap) * 2, inner.y, w, inner.h}, Txt::BrakingFor, buf, kText);

    Txt sound = Txt::SoundNone;
    ImU32 color = kMuted;
    switch (st.sound) {
    case SoundState::BrakeLow:
        sound = Txt::SoundBrake;
        color = kRed;
        break;
    case SoundState::BrakeHigh:
        sound = Txt::SoundBrakeHigh;
        color = kRed;
        break;
    case SoundState::Air:
        sound = Txt::SoundAir;
        color = kAccent;
        break;
    default: break;
    }
    paintMetric(p, ctx, Rect{inner.x + (w + gap) * 3, inner.y, w, inner.h}, Txt::Playing, tx(ctx, sound), color);
}

void paintSettings(const Painter &p, PanelHost &host, Context &ctx, const Rect &card, const PanelSnapshot &st) {
    fillRound(p, card, p.S(10), kWhite);
    strokeRound(p, card, p.S(10), kBorder);
    const float top = card.y + p.S(12);
    text(p, p.f.bold, tx(ctx, Txt::Settings), Rect{card.x + p.S(20), top, p.S(110), p.S(32)}, kText);

    float x = card.x + p.S(130);
    for (Tab t : {Tab::Brake, Tab::Air, Tab::General}) {
        const Txt label = t == Tab::Brake ? Txt::TabBrake : t == Tab::Air ? Txt::TabAir : Txt::TabGeneral;
        bool clicked = false;
        x += tabPill(p, kHitTab + static_cast<int>(t), Rect{x, top, 0, p.S(32)}, tx(ctx, label),
                     static_cast<int>(t) == ctx.prefs.tab, &clicked) + p.S(4);
        if (clicked) setTab(ctx, static_cast<int>(t));
    }

    const float right = card.x + card.w - p.S(16);
    if (button(p, kHitSave, Rect{right - p.S(110), top, p.S(110), p.S(32)}, tx(ctx, Txt::Save), st.unsaved,
               BtnStyle::Primary)) {
        sendPending(host, ctx, true);
        submit(host, ctx, "salvar");
    }
    if (button(p, kHitDefaults, Rect{right - p.S(110) - p.S(10) - p.S(160), top, p.S(160), p.S(32)},
               tx(ctx, Txt::RestoreDefaults), true, BtnStyle::Secondary)) {
        ctx.local.clear();
        ctx.toSend.clear();
        ctx.dragging.clear();
        submit(host, ctx, "padrao");
    }

    const std::vector<Row> rows = rowsFor(static_cast<Tab>(ctx.prefs.tab));
    if (rows.empty()) return;
    const float rowsTop = top + p.S(48);
    const float rowH = std::min(p.S(42), (card.y + card.h - p.S(6) - rowsTop) / static_cast<float>(rows.size()));
    const float labelW = std::min(p.S(290), card.w * 0.38f);
    const float valueW = p.S(96);
    const ImGuiIO &io = ImGui::GetIO();
    const bool english = ctx.prefs.lang == Lang::En;

    float y = rowsTop;
    for (size_t i = 0; i < rows.size(); ++i, y += rowH) {
        const Row &r = rows[i];
        const std::string id = r.meta->id;
        const Rect row{card.x + p.S(8), y, card.w - p.S(16), rowH - p.S(2)};
        const Rect track{card.x + p.S(20) + labelW + p.S(12), y, card.w - labelW - valueW - p.S(64), rowH};
        const bool dragging = ctx.dragging == id;

        // Faixa inteira da linha: hover/selecao e roda do mouse. O trilho (abaixo) fica por cima.
        const HitState line = hit(kHitSlider + 50 + static_cast<int>(i), row, true, true);
        const HitState slider = hit(kHitSlider + static_cast<int>(i),
                                    Rect{track.x - p.S(10), track.y, track.w + p.S(20), track.h}, true, true);
        if (line.hovered || slider.hovered || slider.held) {
            if (io.MouseWheel != 0.0f) nudge(host, ctx, st.config, r, io.MouseWheel > 0 ? 1 : -1, true);
        }
        if (line.clicked) {
            ctx.selected = static_cast<int>(i);
            ctx.selectionVisible = true;
        }
        if (slider.held) {
            ctx.selected = static_cast<int>(i);
            ctx.selectionVisible = false; // o mouse manda agora
            ctx.dragging = id;
            const double fraction = std::clamp(static_cast<double>((io.MousePos.x - track.x) / std::max(1.0f, track.w)),
                                               0.0, 1.0);
            const double display = (r.field->low + (r.field->high - r.field->low) * fraction) * r.meta->scale;
            setValue(host, ctx, st.config, r, snap(r, display), false);
        } else if (dragging) {
            ctx.dragging.clear();
            setValue(host, ctx, st.config, r, shownValue(ctx, st.config, r), true); // soltou: envia o final
        }

        const bool selected = ctx.selectionVisible && static_cast<int>(i) == ctx.selected;
        if (selected) fillRound(p, row, p.S(8), kSelected);
        else if (line.hovered || slider.hovered || dragging) fillRound(p, row, p.S(8), kBg);

        const double value = shownValue(ctx, st.config, r);
        text(p, p.f.regular, ipc::narrow(english ? r.meta->en : r.meta->pt), Rect{card.x + p.S(20), y, labelW, rowH}, kText);

        // trilho + preenchimento + botao
        const float fraction = static_cast<float>(std::clamp((value - r.field->low) / (r.field->high - r.field->low), 0.0, 1.0));
        const float cy = y + rowH * 0.5f, rh = p.S(6);
        fillRound(p, Rect{track.x, cy - rh * 0.5f, track.w, rh}, rh * 0.5f, kGraySoft);
        if (fraction > 0) fillRound(p, Rect{track.x, cy - rh * 0.5f, track.w * fraction, rh}, rh * 0.5f, kAccent);
        const bool hot = slider.held || slider.hovered || dragging;
        const float knob = hot ? p.S(9) : p.S(8);
        const ImVec2 kc(track.x + track.w * fraction, cy);
        p.dl->AddCircleFilled(kc, knob, kWhite, 24);
        p.dl->AddCircle(kc, knob, hot ? kAccentHover : kAccent, 24, p.S(2.5f));

        const bool changed = std::fabs(value - static_cast<double>(defaults().*(r.field->member))) > 1e-6;
        text(p, changed ? p.f.bold : p.f.regular, valueText(r, value),
             Rect{card.x + card.w - valueW - p.S(20), y, valueW, rowH}, changed ? kAccent : kMuted, Align::Right);
    }
}

void paintMessage(const Painter &p, const Context &ctx, const Rect &r, const PanelSnapshot &st) {
    std::string msg = ctx.localMessage;
    bool ok = ctx.localOk;
    if (!st.soundsLoaded) {
        msg = tx(ctx, Txt::NoSound) + st.soundError;
        ok = false;
    } else if (msg.empty() && ctx.messageArmed.load()) {
        msg = st.lastCommandResult;
        ok = st.lastCommandOk;
    }
    if (msg.empty()) return;
    fillRound(p, r, p.S(8), ok ? kGreenSoft : kRedSoft);
    text(p, p.f.bold, msg, Rect{r.x + p.S(14), r.y, r.w - p.S(28), r.h}, ok ? kGreen : kRed);
}

void paintFooter(const Painter &p, Context &ctx, const Rect &r, const PanelSnapshot &st) {
    float x = r.x + r.w;
    const float h = p.S(34), y = r.y + (r.h - h) / 2;
    x -= p.S(120);
    if (button(p, kHitClose, Rect{x, y, p.S(120), h}, tx(ctx, Txt::Close), true, BtnStyle::Primary)) {
        ctx.closeRequested = true;
    }
    x -= p.S(66);
    if (button(p, kHitLang, Rect{x, y, p.S(56), h}, ctx.prefs.lang == Lang::Pt ? "EN" : "PT", true,
               BtnStyle::Secondary)) {
        ctx.prefs.lang = ctx.prefs.lang == Lang::Pt ? Lang::En : Lang::Pt;
        ctx.prefs.save();
    }

    const std::string left = std::string("plugin " AIRBRAKE_AUDIO_VERSION "  \xC2\xB7  ") +
                             tx(ctx, st.unsaved ? Txt::Unsaved : Txt::AllSaved);
    const float w = x - r.x - p.S(12);
    text(p, st.unsaved ? p.f.bold : p.f.regular, left, Rect{r.x, r.y + p.S(4), w, p.S(22)},
         st.unsaved ? kAmber : kMuted);
    text(p, p.f.tiny, tx(ctx, Txt::KeysHint), Rect{r.x, r.y + p.S(26), w, p.S(20)}, kGray);
}

} // namespace

void flush(PanelHost &host, Context &ctx) {
    ctx.dragging.clear();
    if (!ctx.toSend.empty()) sendPending(host, ctx, true);
}

Fonts loadFonts(float scale) {
    Fonts f;
    ImGuiIO &io = ImGui::GetIO();
    io.Fonts->Clear();
    static const ImWchar ranges[] = {0x0020, 0x00FF, 0x2010, 0x2027, 0x2190, 0x2195, 0};
    const char *regularPath = "C:\\Windows\\Fonts\\segoeui.ttf";
    const char *boldPath = "C:\\Windows\\Fonts\\segoeuib.ttf";
    const bool have = GetFileAttributesA(regularPath) != INVALID_FILE_ATTRIBUTES &&
                      GetFileAttributesA(boldPath) != INVALID_FILE_ATTRIBUTES;
    auto add = [&](const char *path, float px) -> ImFont * {
        ImFontConfig cfg;
        cfg.OversampleH = 2;
        cfg.OversampleV = 1;
        return io.Fonts->AddFontFromFileTTF(path, px * scale, &cfg, ranges);
    };
    if (have) {
        f.regular = add(regularPath, 13.5f);
        f.bold = add(boldPath, 13.5f);
        f.title = add(boldPath, 18.0f);
        f.mid = add(boldPath, 22.0f);
        f.tiny = add(regularPath, 12.0f);
        f.unicode = f.regular && f.bold && f.title && f.mid && f.tiny;
    }
    if (!f.unicode) {
        // Sem Segoe UI (ou falhou): fonte embutida do ImGui, sem reticencias.
        io.Fonts->Clear();
        ImFontConfig cfg;
        cfg.SizePixels = 13.0f * scale;
        ImFont *d = io.Fonts->AddFontDefault(&cfg);
        f.regular = f.bold = f.title = f.mid = f.tiny = d;
        f.unicode = false;
    }
    io.FontDefault = f.regular;
    return f;
}

void draw(const PanelSnapshot &st, PanelHost &host, Context &ctx, const Fonts &fonts, float scale) {
    if (!ctx.prefsLoaded) {
        ctx.prefs.load();
        ctx.prefsLoaded = true;
    }
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    ImDrawList *bgl = ImGui::GetBackgroundDrawList();
    bgl->AddRectFilled(ImVec2(0, 0), display, IM_COL32(0, 0, 0, 120)); // escurece o jogo atras

    // Janela do tamanho do app externo (900 x 700 em 1080p), limitada pela tela.
    const float w = std::min(900.0f * scale, display.x - 24.0f);
    const float h = std::min(700.0f * scale, display.y - 24.0f);
    ImGui::SetNextWindowPos(ImVec2(std::floor((display.x - w) * 0.5f), std::floor((display.y - h) * 0.5f)));
    ImGui::SetNextWindowSize(ImVec2(w, h));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                                   ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
                                   ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoNav;
    ImGui::SetNextWindowFocus();
    ImGui::Begin("##airbrake_audio", nullptr, flags);
    ImGui::PushFont(fonts.regular);

    Painter p{ImGui::GetWindowDrawList(), fonts, scale};
    const ImVec2 o = ImGui::GetWindowPos();
    const Rect win{o.x, o.y, w, h};

    updateLocals(ctx, st.config);
    handleKeys(host, ctx, st);

    fillRound(p, win, p.S(10), kBg);
    strokeRound(p, win, p.S(10), rgb(0, 0, 0, 90), 1.0f);
    paintHeader(p, ctx, win, st);

    const float m = p.S(20);
    const float footerH = p.S(52);
    const Rect footer{win.x + m, win.y + h - footerH, w - 2 * m, footerH};
    const Rect content{win.x + m, win.y + p.S(56) + m, w - 2 * m, h - p.S(56) - m - footerH - p.S(6)};

    const Rect metrics{content.x, content.y, content.w, p.S(100)};
    paintMetrics(p, ctx, metrics, st);
    const float msgH = p.S(40);
    const Rect settings{content.x, metrics.y + metrics.h + p.S(14), content.w,
                        content.h - metrics.h - p.S(14) - msgH - p.S(12)};
    paintSettings(p, host, ctx, settings, st);
    paintMessage(p, ctx, Rect{content.x, settings.y + settings.h + p.S(12), content.w, msgH}, st);
    paintFooter(p, ctx, footer, st);

    // Envio em lote enquanto arrasta (so os valores que mudaram, no maximo a cada kSendMs).
    if (!ctx.toSend.empty() && GetTickCount64() - ctx.lastSendMs >= kSendMs) sendPending(host, ctx, false);

    ImGui::PopFont();
    ImGui::End();
    ImGui::PopStyleVar(3);
}

} // namespace aa::ui
