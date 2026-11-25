#pragma once
// Logica dos sons: liga o loop do freio quando o pedal passa de threshold_on, segue o pedal com
// fades e, ao soltar, toca a saida de ar proporcional ao tempo de freada. Sem Windows nem SDK:
// recebe o tempo de fora e fala com ISoundOutput, entao roda nos testes.
#include "config.h"
#include "sound_output.h"

namespace aa {

enum class SoundState { None = 0, Brake = 1, Air = 2 };

class BrakeSounds {
public:
    BrakeSounds(ISoundOutput &out, const Config &config) : out_(out), config_(config) {}

    // Uma vez por frame. pedal 0..1, speedKmh >= 0, now em segundos (monotonico).
    void update(float pedal, float speedKmh, double now);

    // Jogo pausou: silencia tudo e esquece a freada em andamento.
    void stopAll();
    // Jogo voltou: evita um dt gigante no primeiro frame.
    void resume(double now) { lastTick_ = now; }

    // Troca os ajustes. Mudar o ganho recalcula os sons, entao silencia antes.
    void applyConfig(const Config &config);
    const Config &config() const { return config_; }

    // Para a tela.
    float heldSeconds(double now) const { return braking_ ? static_cast<float>(now - pressedAt_) : 0.0f; }
    SoundState state() const;

private:
    void updateTrigger(float pedal, double now);
    void updateBrakeVolume(float pedal, float speedKmh, float dt);
    void updateAirRelease(double now);
    void playAirRelease(double heldMs, double now);

    ISoundOutput &out_;
    Config config_;

    double lastTick_ = 0.0;
    bool started_ = false;

    bool braking_ = false;
    bool brakeVoiceRunning_ = false;
    float brakeVolume_ = 0.0f;
    double pressedAt_ = 0.0;
    double lastAirAt_ = -1e9;

    bool airPlaying_ = false;
    bool airIsCut_ = false;   // fecha antes do fim
    double airStartedAt_ = 0.0;
    double airCutAt_ = 0.0;   // segundos apos o inicio em que o corte termina
    double airEndAt_ = 0.0;   // segundos apos o inicio em que o som inteiro termina
    float airVolume_ = 0.0f;
};

} // namespace aa
