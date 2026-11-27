#pragma once
// Preferencias do app e a janela de Configuracoes (atalho no jogo, gravado no airbrake_audio.ini).
#include "prefs.h"
#include "texts.h"

#include <windows.h>

#include <string>

namespace aa::app {

// Abre a janela de configuracoes (modal). configPath vazio = plugin nunca conectou.
// Devolve true se gravou o atalho no .ini (o chamador manda "recarregar_atalho").
bool showSettingsDialog(HWND owner, const std::wstring &configPath, Lang lang);

} // namespace aa::app
