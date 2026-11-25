#pragma once
// Leitura de WAV PCM 16 bits (mono/estereo) e o ganho com limitador suave.
#include <windows.h>
#include <mmreg.h>

#include <cstdint>
#include <string>
#include <vector>

namespace aa {

struct Wav {
    WAVEFORMATEX format{};
    std::vector<int16_t> samples; // intercaladas
    uint32_t frames = 0;

    float seconds() const {
        return format.nSamplesPerSec ? static_cast<float>(frames) / static_cast<float>(format.nSamplesPerSec) : 0.0f;
    }
};

// Interpreta os bytes de um arquivo .wav. false + motivo em error.
bool parseWav(const std::vector<uint8_t> &bytes, Wav &wav, std::string &error);
bool loadWavFile(const std::wstring &path, Wav &wav, std::string &error);

// Ganho em dB com limitador suave: passa direto ate 0.8 e depois curva tanh ate 1.0 (nunca estoura).
void applyBoost(std::vector<int16_t> &samples, float gainDb);

} // namespace aa
