#pragma once
// O que cada comando do app faz com os ajustes. Puro (sem arquivo nem thread): o AppBridge
// executa os efeitos (salvar, reler o atalho) e os testes conferem a logica.
#include "command_inbox.h"
#include "config.h"

#include <string>

namespace aa {

struct CommandOutcome {
    bool ok = false;
    std::string message;        // vai para status.json -> last_command.result
    bool configChanged = false; // o jogo precisa pegar os ajustes novos
    bool saveRequested = false; // gravar no .ini
    bool reloadHotkey = false;  // reler [ui] hotkey do .ini
};

CommandOutcome interpretCommand(const Command &cmd, Config &config);

} // namespace aa
