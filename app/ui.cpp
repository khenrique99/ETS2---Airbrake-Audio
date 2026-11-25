#include "ui.h"

#include <algorithm>

namespace aa::app::ui {

using namespace Gdiplus;

const Palette P;

namespace {

void roundRectPath(GraphicsPath &path, const RectF &r, float radius) {
    const float d = std::min(radius * 2, std::min(r.Width, r.Height));
    path.Reset();
    path.AddArc(r.X, r.Y, d, d, 180, 90);
    path.AddArc(r.X + r.Width - d, r.Y, d, d, 270, 90);
    path.AddArc(r.X + r.Width - d, r.Y + r.Height - d, d, d, 0, 90);
    path.AddArc(r.X, r.Y + r.Height - d, d, d, 90, 90);
    path.CloseFigure();
}

} // namespace

void Painter::fillRound(const RectF &r, float radius, const Color &c) {
    GraphicsPath path;
    roundRectPath(path, r, radius);
    SolidBrush b(c);
    gr_.FillPath(&b, &path);
}

void Painter::strokeRound(const RectF &r, float radius, const Color &c, float width) {
    GraphicsPath path;
    roundRectPath(path, r, radius);
    Pen pen(c, width);
    gr_.DrawPath(&pen, &path);
}

void Painter::card(const RectF &r) {
    fillRound(r, S(10), P.card);
    strokeRound(r, S(10), P.border);
}

void Painter::text(const std::wstring &s, const RectF &r, float sizePx, const Color &c, bool bold, Align align,
                   bool vcenter) {
    FontFamily fam(L"Segoe UI");
    Font font(&fam, sizePx, bold ? FontStyleBold : FontStyleRegular, UnitPixel);
    StringFormat fmt;
    fmt.SetAlignment(align == Align::Left     ? StringAlignmentNear
                     : align == Align::Center ? StringAlignmentCenter
                                              : StringAlignmentFar);
    fmt.SetLineAlignment(vcenter ? StringAlignmentCenter : StringAlignmentNear);
    fmt.SetTrimming(StringTrimmingEllipsisCharacter);
    fmt.SetFormatFlags(StringFormatFlagsNoWrap);
    SolidBrush b(c);
    gr_.DrawString(s.c_str(), -1, &font, r, &fmt, &b);
}

float Painter::textWidth(const std::wstring &s, float sizePx, bool bold) {
    FontFamily fam(L"Segoe UI");
    Font font(&fam, sizePx, bold ? FontStyleBold : FontStyleRegular, UnitPixel);
    RectF out;
    gr_.MeasureString(s.c_str(), -1, &font, PointF(0, 0), &out);
    return out.Width;
}

void Painter::button(const RectF &r, const std::wstring &label, int id, bool enabled, BtnStyle style) {
    const bool hot = enabled && hover_ == id;
    const bool down = hot && pressed_ == id;
    Color fill, fg, border;
    if (!enabled) {
        fill = P.graySoft;
        fg = P.gray;
        border = P.graySoft;
    } else if (style == BtnStyle::Primary) {
        fill = hot ? P.accentHover : P.accent;
        fg = P.headerText;
        border = fill;
    } else if (style == BtnStyle::Danger) {
        fill = hot ? Color(255, 170, 25, 35) : P.red;
        fg = P.headerText;
        border = fill;
    } else {
        fill = hot ? P.graySoft : P.card;
        fg = P.text;
        border = P.border;
    }
    RectF rr = r;
    if (down) rr.Offset(0, S(1));
    fillRound(rr, S(6), fill);
    if (style == BtnStyle::Secondary || !enabled) strokeRound(rr, S(6), border);
    text(label, rr, S(13), fg, true, Align::Center);
    addHit(r, id, enabled);
}

void Painter::chip(const RectF &r, const std::wstring &label, const Color &fg, const Color &bg) {
    fillRound(r, r.Height / 2, bg);
    SolidBrush dot(fg);
    const float d = S(8);
    gr_.FillEllipse(&dot, r.X + S(10), r.Y + (r.Height - d) / 2, d, d);
    text(label, RectF(r.X + S(22), r.Y, r.Width - S(26), r.Height), S(12.5f), fg, true);
}

float Painter::tab(const RectF &r, const std::wstring &label, int id, bool active) {
    const float w = textWidth(label, S(13), true) + S(28);
    const RectF rr(r.X, r.Y, w, r.Height);
    if (active) fillRound(rr, rr.Height / 2, P.selected);
    else if (hover_ == id) fillRound(rr, rr.Height / 2, P.bg);
    text(label, rr, S(13), active ? P.accent : P.muted, true, Align::Center);
    addHit(rr, id);
    return w;
}

void Painter::slider(const RectF &track, float fraction, int id, bool active) {
    fraction = std::min(1.0f, std::max(0.0f, fraction));
    const float h = S(6);
    const float cy = track.Y + track.Height / 2;
    const RectF rail(track.X, cy - h / 2, track.Width, h);
    fillRound(rail, h / 2, P.graySoft);
    if (fraction > 0) fillRound(RectF(rail.X, rail.Y, rail.Width * fraction, h), h / 2, P.accent);

    const bool hot = active || hover_ == id;
    const float knob = hot ? S(18) : S(16);
    const float kx = track.X + track.Width * fraction;
    SolidBrush white(P.card);
    Pen ring(hot ? P.accentHover : P.accent, S(2.5f));
    gr_.FillEllipse(&white, kx - knob / 2, cy - knob / 2, knob, knob);
    gr_.DrawEllipse(&ring, kx - knob / 2, cy - knob / 2, knob, knob);
    // Area de clique maior que o trilho (mais facil de pegar).
    addHit(RectF(track.X - S(10), track.Y, track.Width + S(20), track.Height), id);
}

int hitTest(const std::vector<Hit> &hits, int x, int y) {
    for (auto it = hits.rbegin(); it != hits.rend(); ++it) {
        if (it->r.Contains(static_cast<REAL>(x), static_cast<REAL>(y))) return it->enabled ? it->id : 0;
    }
    return 0;
}

const Hit *findHit(const std::vector<Hit> &hits, int id) {
    for (const Hit &h : hits)
        if (h.id == id) return &h;
    return nullptr;
}

} // namespace aa::app::ui
