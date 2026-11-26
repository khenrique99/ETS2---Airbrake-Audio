#include "brake_sounds.h"

#include <algorithm>
#include <cmath>

namespace aa {

namespace {

constexpr BrakeLevel kLevels[kBrakeLevels] = {BrakeLevel::Low, BrakeLevel::High};

} // namespace

void BrakeSounds::update(float pedal, float speedKmh, double now) {
    if (!started_) {
        lastTick_ = now;
        started_ = true;
    }
    // dt limitado: um engasgo do jogo nao pode pular o fade inteiro.
    const float dt = static_cast<float>(std::min(0.1, std::max(0.0, now - lastTick_)));
    lastTick_ = now;

    updateTrigger(pedal, now);
    updateLevel(pedal);
    if (braking_ && level_ == BrakeLevel::High) highHeldS_ += dt;
    updateBrakeVolumes(pedal, speedKmh, dt);
    updateAirRelease(now);
}

void BrakeSounds::stopAll() {
    for (BrakeLevel level : kLevels) {
        out_.brakeStop(level);
        voice(level) = Voice{};
    }
    out_.airStop();
    braking_ = false;
    airPlaying_ = false;
}

void BrakeSounds::applyConfig(const Config &config) {
    const bool boostChanged = config.boostDb != config_.boostDb;
    config_ = config;
    if (boostChanged) {
        stopAll(); // as vozes leem os buffers que serao recalculados
        out_.setBoost(config_.boostDb);
    }
}

SoundState BrakeSounds::state() const {
    if (airPlaying_) return SoundState::Air;
    // No cruzamento mostra o que estiver mais alto.
    const Voice &low = voice(BrakeLevel::Low);
    const Voice &high = voice(BrakeLevel::High);
    const float lowVol = low.running ? low.volume : 0.0f;
    const float highVol = high.running ? high.volume : 0.0f;
    if (std::max(lowVol, highVol) <= 0.01f) return SoundState::None;
    return highVol > lowVol ? SoundState::BrakeHigh : SoundState::BrakeLow;
}

void BrakeSounds::updateTrigger(float pedal, double now) {
    if (!braking_ && pedal >= config_.thresholdOn) {
        braking_ = true;
        pressedAt_ = now;
        highHeldS_ = 0.0f;
        // Pisou direto no fundo: ja comeca no loop alto, sem passar pelo baixo.
        level_ = pedal >= config_.brakeHighFrom ? BrakeLevel::High : BrakeLevel::Low;
        startVoice(level_);
    } else if (braking_ && pedal <= config_.thresholdOff) {
        braking_ = false;
        const double heldMs = (now - pressedAt_) * 1000.0;
        const bool heldLongEnough = heldMs >= static_cast<double>(config_.airMinHoldMs);
        const bool cooledDown = (now - lastAirAt_) * 1000.0 >= static_cast<double>(config_.airCooldownMs);
        // Freada leve (so no loop baixo, ou pouco tempo no alto) nao solta ar. 0 desliga a exigencia.
        const bool hotEnough = config_.airMinHighS <= 0.0f || highHeldS_ >= config_.airMinHighS;
        if (heldLongEnough && cooledDown && hotEnough) {
            playAirRelease(heldMs, now);
            lastAirAt_ = now;
        }
    }
}

// Troca de loop durante a freada. Nada e cortado seco: o loop que sai desce com fade_out e o que
// entra sobe com fade_in (updateBrakeVolumes).
void BrakeSounds::updateLevel(float pedal) {
    if (!braking_) return;
    if (level_ == BrakeLevel::Low && pedal >= config_.brakeHighFrom) {
        level_ = BrakeLevel::High;
        startVoice(level_);
    } else if (level_ == BrakeLevel::High && pedal < config_.brakeHighFrom - kHighHysteresis) {
        level_ = BrakeLevel::Low;
        startVoice(level_);
    }
}

// Um loop que ainda esta saindo em fade continua de onde esta (sem reiniciar o ataque).
void BrakeSounds::startVoice(BrakeLevel level) {
    Voice &v = voice(level);
    if (v.running) return;
    out_.brakeStart(level);
    v.running = true;
    v.volume = 0.0f;
}

// O volume do freio segue o pedal; os fades suavizam cada mudanca (e o cruzamento dos loops).
void BrakeSounds::updateBrakeVolumes(float pedal, float speedKmh, float dt) {
    float target = 0.0f;
    if (braking_ && speedKmh >= config_.minSpeedKmh)
        target = config_.volMin + (config_.volMax - config_.volMin) * std::min(1.0f, pedal);

    for (BrakeLevel level : kLevels) updateVoice(level, level == level_ ? target : 0.0f, dt);
}

void BrakeSounds::updateVoice(BrakeLevel level, float target, float dt) {
    Voice &v = voice(level);
    if (!v.running) return;

    if (v.volume < target) {
        v.volume = config_.fadeInMs > 0.0f ? std::min(target, v.volume + dt / (config_.fadeInMs / 1000.0f)) : target;
    } else {
        v.volume =
            config_.fadeOutMs > 0.0f ? std::max(target, v.volume - dt / (config_.fadeOutMs / 1000.0f)) : target;
    }

    // O loop ativo de uma freada fica vivo mesmo mudo (abaixo da velocidade minima); os outros
    // param ao chegar no silencio.
    const bool active = braking_ && level == level_;
    if (!active && v.volume <= 0.0005f) {
        out_.brakeStop(level);
        v = Voice{};
    } else {
        out_.brakeSetVolume(level, v.volume);
    }
}

// Freada longa -> som inteiro em volume cheio. Curta -> mais baixo e cortado cedo.
void BrakeSounds::playAirRelease(double heldMs, double now) {
    const double fullness = std::min(1.0, heldMs / 1000.0 / static_cast<double>(config_.airFullS));
    const double length = static_cast<double>(out_.airSeconds());
    const double audible = std::min(length, static_cast<double>(config_.airAudibleS));
    const double shortest = static_cast<double>(config_.airShortLenS);

    airVolume_ = config_.airVolume * (config_.airShortVol + (1.0f - config_.airShortVol) * static_cast<float>(fullness));
    airIsCut_ = fullness < 1.0;
    airCutAt_ = shortest + (audible - shortest) * fullness;
    airEndAt_ = length + 0.05;
    airStartedAt_ = now;
    airPlaying_ = true;
    out_.airPlay(airVolume_);
}

// Saida de ar cortada fecha com fade exponencial (-80 dB) em vez de parar seco.
void BrakeSounds::updateAirRelease(double now) {
    if (!airPlaying_) return;
    const double elapsed = now - airStartedAt_;

    if (airIsCut_) {
        const double fade = static_cast<double>(config_.airCutFadeMs) / 1000.0;
        const double fadeStart = airCutAt_ - fade;
        if (elapsed >= airCutAt_) {
            out_.airStop();
            airPlaying_ = false;
        } else if (elapsed >= fadeStart) {
            out_.airSetVolume(airVolume_ * static_cast<float>(std::pow(1e-4, (elapsed - fadeStart) / fade)));
        }
    } else if (elapsed >= airEndAt_) {
        out_.airStop();
        airPlaying_ = false;
    }
}

} // namespace aa
