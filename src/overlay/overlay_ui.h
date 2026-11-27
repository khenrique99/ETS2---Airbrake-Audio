#pragma once
// Desenho do painel (Dear ImGui). So aparencia e comandos; nada de DirectX ou janela aqui.
#include "overlay/panel_host.h"
#include "prefs.h"

#include <imgui.h>

#include <atomic>
#include <map>
#include <string>

namespace aa::ui {

struct Fonts {
    ImFont *regular = nullptr; // 13.5 px
    ImFont *bold = nullptr;
    ImFont *title = nullptr;   // 18 px negrito
    ImFont *mid = nullptr;     // 22 px negrito (valores das metricas)
    ImFont *tiny = nullptr;    // 12 px
    bool unicode = false;      // Segoe UI carregada (reticencias); senao so ASCII/Latin-1
};

// Estado do painel entre quadros.
struct Context {
    struct LocalValue {
        double value;
        unsigned long long until;
    };

    app::AppPrefs prefs; // idioma e aba (o mesmo app.ini do AirbrakeAudio.exe)
    bool prefsLoaded = false;

    int selected = 0;               // linha escolhida (teclado) na aba atual
    bool selectionVisible = false;  // so destaca a linha escolhida depois de usar o teclado

    // Valores mostrados antes de o plugin confirmar, e o que ainda falta enviar.
    std::map<std::string, LocalValue> local;
    std::map<std::string, double> toSend;
    unsigned long long lastSendMs = 0;
    std::string dragging; // id do ajuste sendo arrastado ("" = nenhum)

    std::string localMessage;
    bool localOk = true;
    unsigned long long localMessageAt = 0;
    // O resultado do ultimo comando so aparece depois de um comando deste painel (senao reabrir o painel
    // mostraria o resultado de uma sessao anterior).
    std::atomic<bool> messageArmed{false};

    bool closeRequested = false;

    // Teclado (thread da janela -> thread de render).
    std::atomic<int> keyNav{0};    // +1 baixo, -1 cima (acumula)
    std::atomic<int> keyAdjust{0}; // +1 direita, -1 esquerda (acumula)
    std::atomic<int> keyTab{0};    // +1 proxima aba, -1 anterior
    std::atomic<bool> keySave{false};
};

// Desenha o painel. scale = pixels por "unidade de layout" (1.0 em 1080p).
void draw(const PanelSnapshot &state, PanelHost &host, Context &ctx, const Fonts &fonts, float scale);

// Envia o que ficou pendente (ao fechar o painel).
void flush(PanelHost &host, Context &ctx);

// Cria as fontes (Segoe UI do Windows, se existir) no atlas do ImGui. scale = fator de tela.
Fonts loadFonts(float scale);

} // namespace aa::ui
