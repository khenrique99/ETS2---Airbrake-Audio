#include "audio_engine.h"

#include "shared/ipc.h"

#include <xaudio2.h>

#include <algorithm>

namespace aa {

namespace {

void stopVoice(IXAudio2SourceVoice *voice) {
    if (!voice) return;
    voice->Stop();
    voice->FlushSourceBuffers();
}

const char *levelName(BrakeLevel level) {
    return level == BrakeLevel::High ? "som do freio, temperatura alta" : "som do freio, temperatura baixa";
}

} // namespace

bool AudioEngine::loadBrakeLoop(BrakeLevel level, const std::wstring &path, float loopStartS, float boostDb,
                                std::string &error) {
    BrakeLoop &b = loop(level);
    if (!loadWavFile(path, b.wav, error)) return false;
    b.raw = b.wav.samples;
    applyBoost(b.wav.samples, boostDb);
    b.loopStartFrame =
        std::min(b.wav.frames - 1, static_cast<uint32_t>(loopStartS * static_cast<float>(b.wav.format.nSamplesPerSec)));
    return true;
}

bool AudioEngine::init(const Config &config, const std::wstring &soundDir, std::string &error) {
    if (!loadBrakeLoop(BrakeLevel::Low, soundDir + config.brakeLowFile, config.loopStartLowS, config.boostDb, error)) {
        error = std::string(levelName(BrakeLevel::Low)) + " (" + ipc::narrow(config.brakeLowFile) + "): " + error;
        return false;
    }
    if (!loadBrakeLoop(BrakeLevel::High, soundDir + config.brakeFile, config.loopStartS, config.boostDb, error)) {
        error = std::string(levelName(BrakeLevel::High)) + " (" + ipc::narrow(config.brakeFile) + "): " + error;
        return false;
    }
    if (!loadWavFile(soundDir + config.airFile, air_, error)) {
        error = "som da saida de ar (" + ipc::narrow(config.airFile) + "): " + error;
        return false;
    }
    airRaw_ = air_.samples;
    applyBoost(air_.samples, config.boostDb);

    if (FAILED(XAudio2Create(&xaudio_, 0, XAUDIO2_DEFAULT_PROCESSOR))) {
        error = "XAudio2Create falhou";
        return false;
    }
    if (FAILED(xaudio_->CreateMasteringVoice(&master_))) {
        error = "nenhum dispositivo de saida de audio";
        shutdown();
        return false;
    }
    for (BrakeLoop &b : brake_) {
        if (FAILED(xaudio_->CreateSourceVoice(&b.voice, &b.wav.format))) {
            error = "CreateSourceVoice falhou";
            shutdown();
            return false;
        }
    }
    if (FAILED(xaudio_->CreateSourceVoice(&airVoice_, &air_.format))) {
        error = "CreateSourceVoice falhou";
        shutdown();
        return false;
    }
    return true;
}

void AudioEngine::shutdown() {
    for (BrakeLoop &b : brake_) {
        if (b.voice) b.voice->DestroyVoice();
        b.voice = nullptr;
    }
    if (airVoice_) airVoice_->DestroyVoice();
    if (master_) master_->DestroyVoice();
    if (xaudio_) xaudio_->Release();
    airVoice_ = nullptr;
    master_ = nullptr;
    xaudio_ = nullptr;
}

void AudioEngine::setBoost(float gainDb) {
    for (BrakeLoop &b : brake_) {
        b.wav.samples = b.raw;
        applyBoost(b.wav.samples, gainDb);
    }
    air_.samples = airRaw_;
    applyBoost(air_.samples, gainDb);
}

void AudioEngine::brakeStart(BrakeLevel level) {
    BrakeLoop &b = loop(level);
    if (!b.voice) return;
    stopVoice(b.voice);
    XAUDIO2_BUFFER buffer{};
    buffer.AudioBytes = static_cast<UINT32>(b.wav.samples.size() * sizeof(int16_t));
    buffer.pAudioData = reinterpret_cast<const BYTE *>(b.wav.samples.data());
    buffer.LoopBegin = b.loopStartFrame;
    buffer.LoopLength = 0; // ate o fim do arquivo
    buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
    b.voice->SubmitSourceBuffer(&buffer);
    b.voice->SetVolume(0.0f);
    b.voice->Start();
}

void AudioEngine::brakeSetVolume(BrakeLevel level, float volume) {
    if (IXAudio2SourceVoice *v = loop(level).voice) v->SetVolume(volume);
}

void AudioEngine::brakeStop(BrakeLevel level) { stopVoice(loop(level).voice); }

void AudioEngine::airPlay(float volume) {
    if (!airVoice_) return;
    stopVoice(airVoice_);
    XAUDIO2_BUFFER buffer{};
    buffer.Flags = XAUDIO2_END_OF_STREAM;
    buffer.AudioBytes = static_cast<UINT32>(air_.samples.size() * sizeof(int16_t));
    buffer.pAudioData = reinterpret_cast<const BYTE *>(air_.samples.data());
    airVoice_->SubmitSourceBuffer(&buffer);
    airVoice_->SetVolume(volume);
    airVoice_->Start();
}

void AudioEngine::airSetVolume(float volume) {
    if (airVoice_) airVoice_->SetVolume(volume);
}

void AudioEngine::airStop() { stopVoice(airVoice_); }

} // namespace aa
