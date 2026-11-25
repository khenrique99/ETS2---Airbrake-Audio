#include "wav.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace aa {

namespace {

uint32_t readU32(const uint8_t *p) {
    return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) | (static_cast<uint32_t>(p[2]) << 16) |
           (static_cast<uint32_t>(p[3]) << 24);
}
uint16_t readU16(const uint8_t *p) { return static_cast<uint16_t>(p[0] | (p[1] << 8)); }

constexpr long long kMaxFileBytes = 256LL * 1024 * 1024;

} // namespace

bool parseWav(const std::vector<uint8_t> &bytes, Wav &wav, std::string &error) {
    if (bytes.size() < 44) {
        error = "arquivo pequeno demais";
        return false;
    }
    if (std::memcmp(bytes.data(), "RIFF", 4) != 0 || std::memcmp(bytes.data() + 8, "WAVE", 4) != 0) {
        error = "nao e um arquivo WAV";
        return false;
    }

    uint16_t tag = 0, channels = 0, bits = 0;
    uint32_t rate = 0;
    bool haveFormat = false;
    const uint8_t *data = nullptr;
    size_t dataLength = 0;

    for (size_t offset = 12; offset + 8 <= bytes.size();) {
        size_t chunkSize = readU32(&bytes[offset + 4]);
        const size_t body = offset + 8;
        if (body + chunkSize > bytes.size()) chunkSize = bytes.size() - body; // arquivo truncado
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

    if (!haveFormat || !data) {
        error = "WAV sem bloco fmt/data";
        return false;
    }
    if ((tag != WAVE_FORMAT_PCM && tag != WAVE_FORMAT_EXTENSIBLE) || bits != 16) {
        error = "so WAV PCM 16 bits e suportado";
        return false;
    }
    if (channels < 1 || channels > 2 || rate < 8000 || rate > 192000) {
        error = "numero de canais ou taxa de amostragem nao suportados";
        return false;
    }

    Wav out;
    out.format.wFormatTag = WAVE_FORMAT_PCM;
    out.format.nChannels = channels;
    out.format.nSamplesPerSec = rate;
    out.format.wBitsPerSample = 16;
    out.format.nBlockAlign = static_cast<WORD>(channels * 2);
    out.format.nAvgBytesPerSec = rate * out.format.nBlockAlign;
    out.frames = static_cast<uint32_t>(dataLength / out.format.nBlockAlign);
    if (out.frames == 0) {
        error = "WAV vazio";
        return false;
    }
    out.samples.resize(static_cast<size_t>(out.frames) * channels);
    std::memcpy(out.samples.data(), data, out.samples.size() * sizeof(int16_t));
    wav = std::move(out);
    return true;
}

bool loadWavFile(const std::wstring &path, Wav &wav, std::string &error) {
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        error = "arquivo nao encontrado";
        return false;
    }
    LARGE_INTEGER size{};
    if (!GetFileSizeEx(file, &size) || size.QuadPart > kMaxFileBytes) {
        CloseHandle(file);
        error = "tamanho de arquivo invalido";
        return false;
    }
    std::vector<uint8_t> bytes(static_cast<size_t>(size.QuadPart));
    DWORD read = 0;
    const BOOL ok = bytes.empty() || ReadFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &read, nullptr);
    CloseHandle(file);
    if (!ok || read != bytes.size()) {
        error = "falha ao ler o arquivo";
        return false;
    }
    return parseWav(bytes, wav, error);
}

void applyBoost(std::vector<int16_t> &samples, float gainDb) {
    if (gainDb <= 0.0f) return;
    const float gain = std::pow(10.0f, gainDb / 20.0f);
    constexpr float kKnee = 0.8f;
    for (int16_t &sample : samples) {
        float x = static_cast<float>(sample) / 32768.0f * gain;
        float magnitude = std::fabs(x);
        if (magnitude > kKnee) {
            magnitude = kKnee + (1.0f - kKnee) * std::tanh((magnitude - kKnee) / (1.0f - kKnee));
            x = x < 0.0f ? -magnitude : magnitude;
        }
        sample = static_cast<int16_t>(std::lround(std::max(-1.0f, std::min(1.0f, x)) * 32767.0f));
    }
}

} // namespace aa
