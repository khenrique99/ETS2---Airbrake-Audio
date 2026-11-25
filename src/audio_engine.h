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

    void brakeStart() override;
    void brakeSetVolume(float volume) override;
    void brakeStop() override;
    void airPlay(float volume) override;
    void airSetVolume(float volume) override;
    void airStop() override;
    float airSeconds() const override { return air_.seconds(); }
    void setBoost(float gainDb) override;

private:
    IXAudio2 *xaudio_ = nullptr;
    IXAudio2MasteringVoice *master_ = nullptr;
    IXAudio2SourceVoice *brakeVoice_ = nullptr;
    IXAudio2SourceVoice *airVoice_ = nullptr;
    Wav brake_, air_;
    std::vector<int16_t> brakeRaw_, airRaw_; // amostras antes do ganho
    uint32_t loopStartFrame_ = 0;
};

} // namespace aa
