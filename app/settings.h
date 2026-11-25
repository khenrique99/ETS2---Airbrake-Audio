#pragma once
// Preferencias do app e a janela de Configuracoes (atalho no jogo, gravado no airbrake_audio.ini).
#include "texts.h"

#include <windows.h>

#include <string>

namespace aa::app {

// Preferencias do proprio app: Documentos\ETS2AirbrakeAudio\app.ini
struct AppPrefs {
    Lang lang = Lang::Pt;
    bool topmost = false;
    int tab = 0;                 // aba de ajustes aberta por ultimo
    std::wstring lastConfigPath; // ultimo airbrake_audio.ini informado pelo plugin

    void load();
    bool save() const;
};

// Abre a janela de configuracoes (modal). configPath vazio = plugin nunca conectou.
// Devolve true se gravou o atalho no .ini (o chamador manda "recarregar_atalho").
bool showSettingsDialog(HWND owner, const std::wstring &configPath, Lang lang);

} // namespace aa::app
