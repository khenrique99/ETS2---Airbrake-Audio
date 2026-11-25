#include "brake_sounds.h"

#include <algorithm>
#include <cmath>

namespace aa {

void BrakeSounds::update(float pedal, float speedKmh, double now) {
    if (!started_) {
        lastTick_ = now;
        started_ = true;
    }
    // dt limitado: um engasgo do jogo nao pode pular o fade inteiro.
    const float dt = static_cast<float>(std::min(0.1, std::max(0.0, now - lastTick_)));
    lastTick_ = now;

    updateTrigger(pedal, now);
    updateBrakeVolume(pedal, speedKmh, dt);
    updateAirRelease(now);
}

void BrakeSounds::stopAll() {
    out_.brakeStop();
    out_.airStop();
    braking_ = false;
    brakeVoiceRunning_ = false;
    brakeVolume_ = 0.0f;
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
    if (brakeVoiceRunning_ && brakeVolume_ > 0.01f) return SoundState::Brake;
    return SoundState::None;
}

void BrakeSounds::updateTrigger(float pedal, double now) {
    if (!braking_ && pedal >= config_.thresholdOn) {
        braking_ = true;
        pressedAt_ = now;
        if (!brakeVoiceRunning_) {
            out_.brakeStart();
            brakeVoiceRunning_ = true;
            brakeVolume_ = 0.0f;
        }
    } else if (braking_ && pedal <= config_.thresholdOff) {
        braking_ = false;
        const double heldMs = (now - pressedAt_) * 1000.0;
        const bool heldLongEnough = heldMs >= static_cast<double>(config_.airMinHoldMs);
        const bool cooledDown = (now - lastAirAt_) * 1000.0 >= static_cast<double>(config_.airCooldownMs);
        if (heldLongEnough && cooledDown) {
            playAirRelease(heldMs, now);
            lastAirAt_ = now;
        }
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

// O volume do freio segue o pedal; os fades suavizam cada mudanca.
void BrakeSounds::updateBrakeVolume(float pedal, float speedKmh, float dt) {
    if (!brakeVoiceRunning_) return;

    float target = 0.0f;
    if (braking_ && speedKmh >= config_.minSpeedKmh)
        target = config_.volMin + (config_.volMax - config_.volMin) * std::min(1.0f, pedal);

    if (brakeVolume_ < target) {
        brakeVolume_ = config_.fadeInMs > 0.0f ? std::min(target, brakeVolume_ + dt / (config_.fadeInMs / 1000.0f))
                                               : target;
    } else {
        brakeVolume_ = config_.fadeOutMs > 0.0f ? std::max(target, brakeVolume_ - dt / (config_.fadeOutMs / 1000.0f))
                                                : target;
    }

    if (!braking_ && brakeVolume_ <= 0.0005f) {
        out_.brakeStop();
        brakeVoiceRunning_ = false;
        brakeVolume_ = 0.0f;
    } else {
        out_.brakeSetVolume(brakeVolume_);
    }
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
