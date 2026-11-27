#pragma once
// O que o painel dentro do jogo precisa do plugin, sem conhecer o AppBridge: ler o estado e mandar
// comandos (os mesmos do command.txt). Implementado pelo AppBridge; thread-safe.
#include "brake_sounds.h"
#include "config.h"

#include <string>

namespace aa {

// Copia por valor do que o painel mostra (o painel desenha em outra thread).
struct PanelSnapshot {
    bool paused = true;
    float pedal = 0.0f;
    float speedKmh = 0.0f;
    float heldS = 0.0f;
    SoundState sound = SoundState::None;

    Config config; // ajustes atuais (podem ainda nao estar salvos)
    bool unsaved = false;
    bool soundsLoaded = true;
    std::string soundError;

    bool lastCommandOk = true;
    std::string lastCommandResult;
};

class PanelHost {
public:
    virtual ~PanelHost() = default;
    virtual PanelSnapshot snapshot() const = 0;
    // "definir brake.volume_max 0.8", "padrao", "salvar" (sem o numero de sequencia).
    // Roda na thread da ponte; o resultado aparece em snapshot().lastCommandResult.
    virtual void submit(const std::string &action) = 0;
};

} // namespace aa
