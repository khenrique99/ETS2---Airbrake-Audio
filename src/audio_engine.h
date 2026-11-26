#pragma once
// Toca os WAV pelo XAudio2 (vem no Windows 10+). O ganho e aplicado nas amostras ao carregar,
// atras de um limitador suave; em tempo real so muda o volume da voz (sempre <= 1).
#include "config.h"
#include "sound_output.h"
#include "wav.h"

#include <string>
#include <vector>

struct IXAudio2;
struct IXAudio2MasteringVoice;
struct IXAudio2SourceVoice;

namespace aa {

class AudioEngine final : public ISoundOutput {
public:
    AudioEngine() = default;
    AudioEngine(const AudioEngine &) = delete;
    AudioEngine &operator=(const AudioEngine &) = delete;
    ~AudioEngine() override { shutdown(); }

    // Carrega os sons de soundDir e abre o dispositivo de audio. false + motivo em error.
    bool init(const Config &config, const std::wstring &soundDir, std::string &error);
    void shutdown();

    void brakeStart(BrakeLevel level) override;
    void brakeSetVolume(BrakeLevel level, float volume) override;
    void brakeStop(BrakeLevel level) override;
    void airPlay(float volume) override;
    void airSetVolume(float volume) override;
    void airStop() override;
    float airSeconds() const override { return air_.seconds(); }
    void setBoost(float gainDb) override;

private:
    // Um loop de freio por nivel (indice = BrakeLevel), cada um com voz propria.
    struct BrakeLoop {
        IXAudio2SourceVoice *voice = nullptr;
        Wav wav;
        std::vector<int16_t> raw; // amostras antes do ganho
        uint32_t loopStartFrame = 0;
    };

    bool loadBrakeLoop(BrakeLevel level, const std::wstring &path, float loopStartS, float boostDb, std::string &error);
    BrakeLoop &loop(BrakeLevel level) { return brake_[static_cast<int>(level)]; }

    IXAudio2 *xaudio_ = nullptr;
    IXAudio2MasteringVoice *master_ = nullptr;
    IXAudio2SourceVoice *airVoice_ = nullptr;
    BrakeLoop brake_[kBrakeLevels];
    Wav air_;
    std::vector<int16_t> airRaw_; // amostras antes do ganho
};

} // namespace aa
