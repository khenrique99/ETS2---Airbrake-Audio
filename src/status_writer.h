#pragma once
#include "brake_sounds.h"
#include "config.h"

#include <string>

namespace aa {

struct StatusInfo {
    bool paused = true;
    float pedal = 0.0f;
    float speedKmh = 0.0f;
    float heldS = 0.0f;
    SoundState sound = SoundState::None;

    const Config *config = nullptr; // ajustes atuais (podem ainda nao estar salvos)
    bool unsaved = false;
    std::string configPath; // airbrake_audio.ini (UTF-8): a tela de Configuracoes grava o atalho la
    bool soundsLoaded = true;
    std::string soundError;

    long long lastCommandSeq = 0; // ultimo comando processado (0 = nenhum)
    bool lastCommandOk = true;
    std::string lastCommandResult;
};

// Monta o status.json (ver shared/ipc.h). Puro: usado pelos testes.
std::string statusJson(const StatusInfo &info, unsigned long long timestampMs);

// Grava Documentos\ETS2AirbrakeAudio\status.json.
void writeStatus(const StatusInfo &info);

} // namespace aa
