#pragma once
// Protocolo entre o plugin (dentro do jogo) e o app externo "Airbrake Audio".
// Compartilhado pelos dois executaveis; nao depende do SCS SDK.
//
// Pasta: Documentos\ETS2AirbrakeAudio\  (a variavel AIRBRAKE_AUDIO_IPC_DIR troca a pasta; usada nos testes)
//
//   status.json  plugin -> app. Regravado pelo menos 1x por segundo (campo "timestamp_ms" =
//                batimento) e a cada kLiveMs enquanto a janela do app existir. .tmp + troca atomica.
//
//   command.txt  app -> plugin. Uma linha por comando:  <seq> <acao> [arg1] [arg2]
//                O app grava em command.txt.tmp e troca atomicamente. O plugin "pega" o arquivo
//                renomeando para command.txt.taken, le e apaga.
//
//   Acoes:  definir <id> <valor>   muda um ajuste na hora (ex.: definir brake.volume_max 0.8)
//           padrao                 volta todos os ajustes ao padrao (sem salvar)
//           salvar                 grava os ajustes atuais no airbrake_audio.ini
//           recarregar_atalho      rele [ui] hotkey do .ini (a tela de Configuracoes grava la)
//
//   O resultado de cada comando volta em status.json -> "last_command": {"seq", "ok", "result"}.

#include <windows.h>
#include <shlobj.h>

#include <cstdlib>
#include <string>

namespace aa::ipc {

constexpr int kProtocolVersion = 1;

// Janela do app: o plugin a encontra pela classe para mostra-la (atalho no jogo).
constexpr const wchar_t *kAppWindowClass = L"ETS2AirbrakeAudioApp";
// plugin -> app: "mostre-se"; wParam = HWND da janela do jogo (para "voltar ao jogo").
constexpr UINT kMsgShowFromGame = WM_APP + 0x41;
constexpr const wchar_t *kFolderName = L"ETS2AirbrakeAudio";
constexpr const wchar_t *kFolderOverrideEnv = L"AIRBRAKE_AUDIO_IPC_DIR";
constexpr const wchar_t *kStatusFile = L"status.json";
constexpr const wchar_t *kCommandFile = L"command.txt";

// Batimento: o plugin regrava status.json pelo menos a cada kHeartbeatMs...
constexpr unsigned kHeartbeatMs = 1000;
// ...e a cada kLiveMs com o app aberto (metricas ao vivo).
constexpr unsigned kLiveMs = 100;
// O app considera o jogo desconectado se o batimento ficar mais velho que isso.
constexpr unsigned kStaleMs = 3500;

// Documentos\ETS2AirbrakeAudio (cria se nao existir). Vazio se falhar.
inline std::wstring folder() {
    wchar_t custom[MAX_PATH] = {};
    if (GetEnvironmentVariableW(kFolderOverrideEnv, custom, MAX_PATH) > 0) {
        CreateDirectoryW(custom, nullptr);
        return custom;
    }
    PWSTR docs = nullptr;
    if (FAILED(SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &docs))) return {};
    std::wstring dir = docs;
    CoTaskMemFree(docs);
    dir += L"\\";
    dir += kFolderName;
    CreateDirectoryW(dir.c_str(), nullptr);
    return dir;
}

inline std::wstring statusPath() {
    const std::wstring d = folder();
    return d.empty() ? d : d + L"\\" + kStatusFile;
}

inline std::wstring commandPath() {
    const std::wstring d = folder();
    return d.empty() ? d : d + L"\\" + kCommandFile;
}

// Tempo Unix em milissegundos (para o batimento).
inline unsigned long long nowUnixMs() {
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    ULARGE_INTEGER u;
    u.LowPart = ft.dwLowDateTime;
    u.HighPart = ft.dwHighDateTime;
    return (u.QuadPart - 116444736000000000ULL) / 10000ULL;
}

// Grava texto num arquivo de forma atomica (tmp + troca).
inline bool writeAtomic(const std::wstring &path, const std::string &text) {
    const std::wstring tmp = path + L".tmp";
    HANDLE h = CreateFileW(tmp.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    DWORD written = 0;
    const BOOL ok = WriteFile(h, text.data(), static_cast<DWORD>(text.size()), &written, nullptr);
    CloseHandle(h);
    if (!ok || written != text.size()) return false;
    return MoveFileExW(tmp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING) != 0;
}

// Le o arquivo inteiro (compartilhado, aceita escrita/troca concorrente).
inline bool readAll(const std::wstring &path, std::string &out) {
    HANDLE h = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER size{};
    if (!GetFileSizeEx(h, &size) || size.QuadPart > 4 * 1024 * 1024) {
        CloseHandle(h);
        return false;
    }
    out.resize(static_cast<size_t>(size.QuadPart));
    DWORD read = 0;
    const BOOL ok = out.empty() || ReadFile(h, &out[0], static_cast<DWORD>(out.size()), &read, nullptr);
    CloseHandle(h);
    if (!ok) return false;
    out.resize(read);
    return true;
}

// UTF-8 <-> UTF-16 (caminhos e textos que vao para o JSON / a tela).
inline std::wstring widen(const std::string &utf8) {
    if (utf8.empty()) return {};
    const int n = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
    std::wstring w(static_cast<size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), &w[0], n);
    return w;
}

inline std::string narrow(const std::wstring &w) {
    if (w.empty()) return {};
    const int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), nullptr, 0, nullptr, nullptr);
    std::string s(static_cast<size_t>(n), '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), &s[0], n, nullptr, nullptr);
    return s;
}

} // namespace aa::ipc
