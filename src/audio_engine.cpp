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

} // namespace

bool AudioEngine::init(const Config &config, const std::wstring &soundDir, std::string &error) {
    if (!loadWavFile(soundDir + config.brakeFile, brake_, error)) {
        error = "som do freio (" + ipc::narrow(config.brakeFile) + "): " + error;
        return false;
    }
    if (!loadWavFile(soundDir + config.airFile, air_, error)) {
        error = "som da saida de ar (" + ipc::narrow(config.airFile) + "): " + error;
        return false;
    }
    brakeRaw_ = brake_.samples;
    airRaw_ = air_.samples;
    applyBoost(brake_.samples, config.boostDb);
    applyBoost(air_.samples, config.boostDb);
    loopStartFrame_ =
        std::min(brake_.frames - 1, static_cast<uint32_t>(config.loopStartS * static_cast<float>(brake_.format.nSamplesPerSec)));

    if (FAILED(XAudio2Create(&xaudio_, 0, XAUDIO2_DEFAULT_PROCESSOR))) {
        error = "XAudio2Create falhou";
        return false;
    }
    if (FAILED(xaudio_->CreateMasteringVoice(&master_))) {
        error = "nenhum dispositivo de saida de audio";
        shutdown();
        return false;
    }
    if (FAILED(xaudio_->CreateSourceVoice(&brakeVoice_, &brake_.format)) ||
        FAILED(xaudio_->CreateSourceVoice(&airVoice_, &air_.format))) {
        error = "CreateSourceVoice falhou";
        shutdown();
        return false;
    }
    return true;
}

void AudioEngine::shutdown() {
    if (brakeVoice_) brakeVoice_->DestroyVoice();
    if (airVoice_) airVoice_->DestroyVoice();
    if (master_) master_->DestroyVoice();
    if (xaudio_) xaudio_->Release();
    brakeVoice_ = airVoice_ = nullptr;
    master_ = nullptr;
    xaudio_ = nullptr;
}

void AudioEngine::setBoost(float gainDb) {
    brake_.samples = brakeRaw_;
    air_.samples = airRaw_;
    applyBoost(brake_.samples, gainDb);
    applyBoost(air_.samples, gainDb);
}

void AudioEngine::brakeStart() {
    if (!brakeVoice_) return;
    stopVoice(brakeVoice_);
    XAUDIO2_BUFFER buffer{};
    buffer.AudioBytes = static_cast<UINT32>(brake_.samples.size() * sizeof(int16_t));
    buffer.pAudioData = reinterpret_cast<const BYTE *>(brake_.samples.data());
    buffer.LoopBegin = loopStartFrame_;
    buffer.LoopLength = 0; // ate o fim do arquivo
    buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
    brakeVoice_->SubmitSourceBuffer(&buffer);
    brakeVoice_->SetVolume(0.0f);
    brakeVoice_->Start();
}

void AudioEngine::brakeSetVolume(float volume) {
    if (brakeVoice_) brakeVoice_->SetVolume(volume);
}

void AudioEngine::brakeStop() { stopVoice(brakeVoice_); }

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
