#pragma once
// Identidade visual compartilhada com o Trailer Control: mesma paleta, cabecalho escuro,
// cartoes brancos arredondados, botoes e "chips" de status, Segoe UI. Desenho com GDI+.
#include <windows.h>

#include <objidl.h>
#include <gdiplus.h>

#include <string>
#include <vector>

namespace aa::app::ui {

using Gdiplus::Color;
using Gdiplus::RectF;

struct Palette {
    Color bg{255, 243, 244, 246};
    Color header{255, 24, 31, 42};
    Color headerText{255, 255, 255, 255};
    Color card{255, 255, 255, 255};
    Color border{255, 222, 226, 231};
    Color text{255, 31, 35, 40};
    Color muted{255, 101, 109, 118};
    Color accent{255, 37, 99, 235};
    Color accentHover{255, 29, 78, 216};
    Color green{255, 26, 127, 55};
    Color greenSoft{255, 220, 245, 228};
    Color red{255, 207, 34, 46};
    Color redSoft{255, 255, 230, 232};
    Color amber{255, 154, 103, 0};
    Color amberSoft{255, 255, 244, 214};
    Color gray{255, 140, 149, 159};
    Color graySoft{255, 234, 238, 242};
    Color selected{255, 232, 240, 254};
};
extern const Palette P;

enum class Align { Left, Center, Right };
enum class BtnStyle { Primary, Danger, Secondary };

struct Hit {
    RectF r;
    int id;
    bool enabled;
};

// Um quadro de pintura: escala de DPI, estado do mouse e as areas clicaveis desenhadas.
class Painter {
public:
    Painter(Gdiplus::Graphics &gr, float scale, int hover, int pressed, std::vector<Hit> &hits)
        : gr_(gr), scale_(scale), hover_(hover), pressed_(pressed), hits_(hits) {}

    float S(float v) const { return v * scale_; }
    Gdiplus::Graphics &graphics() { return gr_; }
    int hover() const { return hover_; }

    void fillRound(const RectF &r, float radius, const Color &c);
    void strokeRound(const RectF &r, float radius, const Color &c, float width = 1.0f);
    void card(const RectF &r); // fundo branco + borda
    void text(const std::wstring &s, const RectF &r, float sizePx, const Color &c, bool bold = false,
              Align align = Align::Left, bool vcenter = true);
    float textWidth(const std::wstring &s, float sizePx, bool bold = false);
    void button(const RectF &r, const std::wstring &label, int id, bool enabled, BtnStyle style);
    void chip(const RectF &r, const std::wstring &label, const Color &fg, const Color &bg);
    // Abas em "pilula"; devolve a largura usada.
    float tab(const RectF &r, const std::wstring &label, int id, bool active);
    // Trilho + preenchimento + botao. fraction 0..1.
    void slider(const RectF &track, float fraction, int id, bool active);
    void addHit(const RectF &r, int id, bool enabled = true) { hits_.push_back({r, id, enabled}); }

private:
    Gdiplus::Graphics &gr_;
    float scale_;
    int hover_, pressed_;
    std::vector<Hit> &hits_;
};

// Ultimos desenhados ficam por cima. 0 = nada.
int hitTest(const std::vector<Hit> &hits, int x, int y);
const Hit *findHit(const std::vector<Hit> &hits, int id);

} // namespace aa::app::ui
