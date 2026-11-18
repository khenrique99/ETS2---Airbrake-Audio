// Audio engine: loads 16-bit PCM WAV files and plays them through XAudio2
// (built into Windows 10+). The boost is baked into the samples at load time
// behind a soft limiter, so it can never clip. Runtime volume (pedal, fades)
// is only the voice volume and always stays <= 1.
#pragma once
#include <windows.h>
#include <xaudio2.h>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include "config.h"

struct Wav {
    WAVEFORMATEX format{};
    std::vector<int16_t> samples;       // interleaved
    uint32_t frames = 0;

    float seconds() const {
        return format.nSamplesPerSec ? float(frames) / float(format.nSamplesPerSec) : 0.0f;
    }
};

inline uint32_t readU32(const uint8_t* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | (uint32_t(p[3]) << 24); }
inline uint16_t readU16(const uint8_t* p) { return uint16_t(p[0] | (p[1] << 8)); }

inline bool loadWav(const std::wstring& path, Wav& wav, std::string& error) {
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) { error = "file not found"; return false; }

    LARGE_INTEGER size{};
    if (!GetFileSizeEx(file, &size) || size.QuadPart < 44 || size.QuadPart > 256LL * 1024 * 1024) {
        CloseHandle(file);
        error = "invalid file size";
        return false;
    }
    std::vector<uint8_t> bytes(size_t(size.QuadPart));
    DWORD bytesRead = 0;
    const BOOL ok = ReadFile(file, bytes.data(), DWORD(bytes.size()), &bytesRead, nullptr);
    CloseHandle(file);
    if (!ok || bytesRead != bytes.size()) { error = "failed to read file"; return false; }

    if (std::memcmp(bytes.data(), "RIFF", 4) != 0 || std::memcmp(bytes.data() + 8, "WAVE", 4) != 0) {
        error = "not a WAV file";
        return false;
    }

    uint16_t tag = 0, channels = 0, bits = 0;
    uint32_t rate = 0;
    bool haveFormat = false;
    const uint8_t* data = nullptr;
    size_t dataLength = 0;

    for (size_t offset = 12; offset + 8 <= bytes.size();) {
        uint32_t chunkSize = readU32(&bytes[offset + 4]);
        const size_t body = offset + 8;
        if (body + chunkSize > bytes.size()) chunkSize = uint32_t(bytes.size() - body);   // truncated file
        if (std::memcmp(&bytes[offset], "fmt ", 4) == 0 && chunkSize >= 16) {
            tag = readU16(&bytes[body]);
            channels = readU16(&bytes[body + 2]);
            rate = readU32(&bytes[body + 4]);
            bits = readU16(&bytes[body + 14]);
            haveFormat = true;
        } else if (std::memcmp(&bytes[offset], "data", 4) == 0) {
            data = &bytes[body];
            dataLength = chunkSize;
        }
        offset = body + chunkSize + (chunkSize & 1);
    }

    if (!haveFormat || !data) { error = "WAV has no fmt/data chunk"; return false; }
    if ((tag != 1 && tag != 0xFFFE) || bits != 16) { error = "only 16-bit PCM is supported"; return false; }
    if (channels < 1 || channels > 2 || rate < 8000 || rate > 192000) {
        error = "unsupported channel count or sample rate";
        return false;
    }

    wav.format = WAVEFORMATEX{};
    wav.format.wFormatTag = WAVE_FORMAT_PCM;
    wav.format.nChannels = channels;
    wav.format.nSamplesPerSec = rate;
    wav.format.wBitsPerSample = 16;
    wav.format.nBlockAlign = WORD(channels * 2);
    wav.format.nAvgBytesPerSec = rate * wav.format.nBlockAlign;
    wav.frames = uint32_t(dataLength / wav.format.nBlockAlign);
    if (wav.frames == 0) { error = "WAV is empty"; return false; }

    wav.samples.resize(size_t(wav.frames) * channels);
    std::memcpy(wav.samples.data(), data, wav.samples.size() * sizeof(int16_t));
    return true;
}

// Gain plus a soft limiter: passes through below 0.8, then a tanh curve up to 1.0.
inline void applyBoost(Wav& wav, float gainDb) {
    if (gainDb <= 0.0f) return;
    const float gain = std::pow(10.0f, gainDb / 20.0f);
    const float knee = 0.8f;
    for (int16_t& sample : wav.samples) {
        float x = float(sample) / 32768.0f * gain;
        float magnitude = std::fabs(x);
        if (magnitude > knee) {
            magnitude = knee + (1.0f - knee) * std::tanh((magnitude - knee) / (1.0f - knee));
            x = x < 0.0f ? -magnitude : magnitude;
        }
        sample = int16_t(std::lround(std::max(-1.0f, std::min(1.0f, x)) * 32767.0f));
    }
}

class Engine {
public:
    bool init(const Config& config, const std::wstring& soundDir, std::string& error) {
        if (!loadWav(soundDir + config.brakeFile, brake_, error)) {
            error = "brake sound (" + toAscii(config.brakeFile) + "): " + error;
            return false;
        }
        if (!loadWav(soundDir + config.airFile, air_, error)) {
            error = "air release sound (" + toAscii(config.airFile) + "): " + error;
            return false;
        }
        brakeRaw_ = brake_.samples;
        airRaw_ = air_.samples;
        applyBoost(brake_, config.boostDb);
        applyBoost(air_, config.boostDb);
        loopStartFrame_ = std::min(brake_.frames - 1,
                                   uint32_t(config.loopStartS * float(brake_.format.nSamplesPerSec)));

        if (FAILED(XAudio2Create(&xaudio_, 0, XAUDIO2_DEFAULT_PROCESSOR))) {
            error = "XAudio2Create failed";
            return false;
        }
        if (FAILED(xaudio_->CreateMasteringVoice(&masterVoice_))) {
            error = "no audio output device";
            shutdown();
            return false;
        }
        if (FAILED(xaudio_->CreateSourceVoice(&brakeVoice_, &brake_.format)) ||
            FAILED(xaudio_->CreateSourceVoice(&airVoice_, &air_.format))) {
            error = "CreateSourceVoice failed";
            shutdown();
            return false;
        }
        return true;
    }

    void shutdown() {
        if (brakeVoice_)  { brakeVoice_->DestroyVoice();  brakeVoice_ = nullptr; }
        if (airVoice_)    { airVoice_->DestroyVoice();    airVoice_ = nullptr; }
        if (masterVoice_) { masterVoice_->DestroyVoice(); masterVoice_ = nullptr; }
        if (xaudio_)      { xaudio_->Release();           xaudio_ = nullptr; }
    }

    float airSeconds() const { return air_.seconds(); }

    // Re-bakes the gain from the original samples. Both voices must be stopped first
    // (they read the sample buffers), so the caller stops the sounds before calling it.
    void setBoost(float gainDb) {
        brake_.samples = brakeRaw_;
        air_.samples = airRaw_;
        applyBoost(brake_, gainDb);
        applyBoost(air_, gainDb);
    }

    // Brake: continuous loop that restarts at loop_start_s.
    void brakeStart() {
        if (!brakeVoice_) return;
        brakeVoice_->Stop();
        brakeVoice_->FlushSourceBuffers();

        XAUDIO2_BUFFER buffer{};
        buffer.AudioBytes = UINT32(brake_.samples.size() * sizeof(int16_t));
        buffer.pAudioData = reinterpret_cast<const BYTE*>(brake_.samples.data());
        buffer.LoopBegin = loopStartFrame_;
        buffer.LoopLength = 0;                      // loop to the end of the file
        buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
        brakeVoice_->SubmitSourceBuffer(&buffer);
        brakeVoice_->SetVolume(0.0f);
        brakeVoice_->Start();
    }
    void brakeSetVolume(float volume) { if (brakeVoice_) brakeVoice_->SetVolume(volume); }
    void brakeStop() {
        if (!brakeVoice_) return;
        brakeVoice_->Stop();
        brakeVoice_->FlushSourceBuffers();
    }

    // Air release: plays once; the caller handles early cuts via airSetVolume / airStop.
    void airPlay(float volume) {
        if (!airVoice_) return;
        airVoice_->Stop();
        airVoice_->FlushSourceBuffers();

        XAUDIO2_BUFFER buffer{};
        buffer.Flags = XAUDIO2_END_OF_STREAM;
        buffer.AudioBytes = UINT32(air_.samples.size() * sizeof(int16_t));
        buffer.pAudioData = reinterpret_cast<const BYTE*>(air_.samples.data());
        airVoice_->SubmitSourceBuffer(&buffer);
        airVoice_->SetVolume(volume);
        airVoice_->Start();
    }
    void airSetVolume(float volume) { if (airVoice_) airVoice_->SetVolume(volume); }
    void airStop() {
        if (!airVoice_) return;
        airVoice_->Stop();
        airVoice_->FlushSourceBuffers();
    }

private:
    static std::string toAscii(const std::wstring& text) {
        std::string result;
        for (wchar_t c : text) result += (c < 128 ? char(c) : '?');
        return result;
    }

    IXAudio2* xaudio_ = nullptr;
    IXAudio2MasteringVoice* masterVoice_ = nullptr;
    IXAudio2SourceVoice* brakeVoice_ = nullptr;
    IXAudio2SourceVoice* airVoice_ = nullptr;
    Wav brake_, air_;
    std::vector<int16_t> brakeRaw_, airRaw_;   // samples before the boost
    uint32_t loopStartFrame_ = 0;
};
