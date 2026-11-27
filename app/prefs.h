#pragma once
// Preferencias da tela (idioma, aba aberta, sempre no topo): Documentos\ETS2AirbrakeAudio\app.ini.
// Compartilhadas pelo AirbrakeAudio.exe e pelo painel dentro do jogo (plugin).
#include "texts.h"

#include <string>

namespace aa::app {

struct AppPrefs {
    Lang lang = Lang::Pt;
    bool topmost = false;
    int tab = 0;                 // aba de ajustes aberta por ultimo
    std::wstring lastConfigPath; // ultimo airbrake_audio.ini informado pelo plugin

    void load();
    bool save() const;
};

} // namespace aa::app
