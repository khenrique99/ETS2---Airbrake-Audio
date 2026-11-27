// "Jogo falso": carrega a DLL do plugin pelo SDK de telemetria, simula pedal/velocidade e
// conversa com o plugin como o app faria (command.txt / status.json). Roda sem o ETS2.
// Uso: fake_game <caminho da airbrake_audio.dll> --selftest
//      fake_game <caminho da airbrake_audio.dll> --overlay <foto.bmp> [--mouse]   (painel dentro do jogo; precisa de tela;
//        --mouse leva o cursor real ate os controles por uns instantes)
#include "json.h"
#include "scs_sdk_min.h"
#include "shared/ipc.h"

#include <windows.h>

#include <d3d11.h>
#include <dxgi.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace {

int g_failures = 0;

void check(bool cond, const char *what, int line) {
    if (!cond) {
        ++g_failures;
        std::printf("FALHOU (linha %d): %s\n", line, what);
    }
}
#define CHECK(c) check((c), #c, __LINE__)

// ---- o "jogo": guarda o que o plugin registrou
std::map<scs_event_t, scs_telemetry_event_callback_t> g_events;
struct ChannelReg {
    scs_telemetry_channel_callback_t callback;
    scs_context_t context;
};
std::map<std::string, ChannelReg> g_channels;
std::string g_log;

void gameLog(const scs_log_type_t, const scs_string_t message) {
    g_log += message;
    g_log += '\n';
}
scs_result_t registerEvent(const scs_event_t e, const scs_telemetry_event_callback_t cb, const scs_context_t) {
    g_events[e] = cb;
    return SCS_RESULT_ok;
}
scs_result_t unregisterEvent(const scs_event_t) { return SCS_RESULT_ok; }
scs_result_t registerChannel(const scs_string_t name, const scs_u32_t, const scs_value_type_t,
                             const scs_u32_t, const scs_telemetry_channel_callback_t cb, const scs_context_t ctx) {
    g_channels[name] = {cb, ctx};
    return SCS_RESULT_ok;
}
scs_result_t unregisterChannel(const scs_string_t, const scs_u32_t, const scs_value_type_t) { return SCS_RESULT_ok; }

void setChannel(const char *name, float value) {
    const ChannelReg &c = g_channels[name];
    scs_value_t v{};
    v.type = SCS_VALUE_TYPE_float;
    v.value_float = value;
    c.callback(name, SCS_U32_NIL, &v, c.context);
}

void fire(scs_event_t e) {
    if (g_events.count(e)) g_events[e](e, nullptr, nullptr);
}

void frames(int n) {
    for (int i = 0; i < n; ++i) {
        fire(SCS_TELEMETRY_EVENT_frame_end);
        Sleep(16);
    }
}

// ---- conversa com o plugin
bool readStatus(aa::json::Value &out) {
    std::string text;
    return aa::ipc::readAll(aa::ipc::statusPath(), text) && aa::json::parse(text, out);
}

long long g_seq = 0;
void sendCommand(const std::string &action) {
    std::string text;
    aa::ipc::readAll(aa::ipc::commandPath(), text);
    text += std::to_string(++g_seq) + " " + action + "\n";
    aa::ipc::writeAtomic(aa::ipc::commandPath(), text);
}

// Espera o plugin responder o comando (ele publica a cada 50 ms quando ha comando).
bool waitReply(long long seq, aa::json::Value &status) {
    for (int i = 0; i < 100; ++i) {
        Sleep(50);
        frames(1); // o jogo consome as configuracoes novas no frame_end
        if (readStatus(status) && status["last_command"]["seq"].asInt(0) >= seq) return true;
    }
    return false;
}

const aa::json::Value *findField(const aa::json::Value &status, const std::string &id) {
    static aa::json::Value holder;
    const aa::json::Value &fields = status["fields"];
    for (size_t i = 0; i < fields.size(); ++i) {
        if (fields[i]["id"].asString() == id) {
            holder = fields[i];
            return &holder;
        }
    }
    return nullptr;
}

std::wstring makeTempDir() {
    wchar_t tmp[MAX_PATH];
    GetTempPathW(MAX_PATH, tmp);
    const std::wstring dir = std::wstring(tmp) + L"aa_selftest_" + std::to_wstring(GetCurrentProcessId());
    CreateDirectoryW(dir.c_str(), nullptr);
    return dir;
}

using InitFn = scs_result_t (*)(scs_u32_t, const scs_telemetry_init_params_v100_t *);
using ShutdownFn = void (*)();

scs_telemetry_init_params_v100_t makeParams() {
    scs_telemetry_init_params_v100_t params{};
    params.common.game_name = "Fake";
    params.common.game_id = "fake";
    params.common.log = gameLog;
    params.register_for_event = registerEvent;
    params.unregister_from_event = unregisterEvent;
    params.register_for_channel = registerChannel;
    params.unregister_from_channel = unregisterChannel;
    return params;
}

void selftest(HMODULE dll) {
    auto init = reinterpret_cast<InitFn>(GetProcAddress(dll, "scs_telemetry_init"));
    auto shutdown = reinterpret_cast<ShutdownFn>(GetProcAddress(dll, "scs_telemetry_shutdown"));
    CHECK(init && shutdown);
    if (!init || !shutdown) return;

    const scs_telemetry_init_params_v100_t params = makeParams();

    CHECK(init(SCS_MAKE_VERSION(9, 9), &params) == SCS_RESULT_unsupported); // versao nao suportada
    CHECK(init(SCS_TELEMETRY_VERSION_1_01, &params) == SCS_RESULT_ok);
    CHECK(g_events.count(SCS_TELEMETRY_EVENT_frame_end) == 1);
    CHECK(g_channels.count("truck.input.brake") == 1 && g_channels.count("truck.speed") == 1);

    // O plugin carrega mesmo sem placa de som (CI): so mostra o motivo na tela.
    fire(SCS_TELEMETRY_EVENT_started);
    setChannel("truck.speed", 20.0f);
    setChannel("truck.input.brake", 0.8f);
    frames(5);
    setChannel("truck.input.brake", 0.0f);
    frames(5);

    aa::json::Value status;
    bool published = false;
    for (int i = 0; i < 60 && !published; ++i) {
        Sleep(50);
        published = readStatus(status);
    }
    CHECK(published);
    CHECK(status["protocol"].asInt(0) == aa::ipc::kProtocolVersion);
    CHECK(!status["plugin_version"].asString().empty());
    CHECK(status["timestamp_ms"].asNumber(0) > 0);
    CHECK(status["fields"].size() > 10);

    // definir -> o plugin confirma e o valor volta no status
    sendCommand("definir brake.volume_max 0.5");
    CHECK(waitReply(g_seq, status));
    CHECK(status["last_command"]["ok"].asBool(false));
    const aa::json::Value *f = findField(status, "brake.volume_max");
    CHECK(f && f->operator[]("value").asNumber(0) > 0.49 && f->operator[]("value").asNumber(0) < 0.51);
    CHECK(status["unsaved"].asBool(false));

    // valor fora da faixa e limitado
    sendCommand("definir brake.volume_max 7");
    CHECK(waitReply(g_seq, status));
    f = findField(status, "brake.volume_max");
    CHECK(f && f->operator[]("value").asNumber(0) <= 1.0);

    // erros: ajuste desconhecido, valor invalido, acao desconhecida
    sendCommand("definir nao.existe 1");
    CHECK(waitReply(g_seq, status) && !status["last_command"]["ok"].asBool(true));
    sendCommand("definir brake.volume_max abc");
    CHECK(waitReply(g_seq, status) && !status["last_command"]["ok"].asBool(true));
    sendCommand("formatar");
    CHECK(waitReply(g_seq, status) && !status["last_command"]["ok"].asBool(true));

    // padrao
    sendCommand("padrao");
    CHECK(waitReply(g_seq, status) && status["last_command"]["ok"].asBool(false));
    f = findField(status, "brake.volume_max");
    CHECK(f && f->operator[]("value").asNumber(0) > 0.99);

    // jogo pausa e volta
    fire(SCS_TELEMETRY_EVENT_paused);
    fire(SCS_TELEMETRY_EVENT_started);
    frames(3);

    shutdown();
    CHECK(g_log.find("[AirbrakeAudio]") != std::string::npos);
}

// ---- teste do painel dentro do jogo: janela DX11 de verdade; abre com Ctrl + =, tira "foto" da tela,
// ajusta controles pelo teclado (e, se pedido, clica num ponto) e confere o status.json. Sem o ETS2.
LRESULT CALLBACK testWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

bool saveScreenBmp(HWND hwnd, const std::wstring &path) {
    RECT rc{};
    GetClientRect(hwnd, &rc);
    POINT origin{0, 0};
    ClientToScreen(hwnd, &origin);
    const int w = rc.right, h = rc.bottom;
    HDC screen = GetDC(nullptr);
    HDC mem = CreateCompatibleDC(screen);
    HBITMAP bmp = CreateCompatibleBitmap(screen, w, h);
    HGDIOBJ old = SelectObject(mem, bmp);
    BitBlt(mem, 0, 0, w, h, screen, origin.x, origin.y, SRCCOPY | CAPTUREBLT);
    BITMAPINFOHEADER bi{sizeof(bi), w, -h, 1, 32, BI_RGB, 0, 0, 0, 0, 0};
    std::vector<unsigned char> px(static_cast<size_t>(w) * h * 4);
    GetDIBits(mem, bmp, 0, static_cast<UINT>(h), px.data(), reinterpret_cast<BITMAPINFO *>(&bi), DIB_RGB_COLORS);
    SelectObject(mem, old);
    DeleteObject(bmp);
    DeleteDC(mem);
    ReleaseDC(nullptr, screen);

    BITMAPFILEHEADER fh{0x4D42, static_cast<DWORD>(sizeof(fh) + sizeof(bi) + px.size()), 0, 0, sizeof(fh) + sizeof(bi)};
    FILE *f = _wfopen(path.c_str(), L"wb");
    if (!f) return false;
    fwrite(&fh, sizeof(fh), 1, f);
    fwrite(&bi, sizeof(bi), 1, f);
    fwrite(px.data(), 1, px.size(), f);
    fclose(f);
    return true;
}

// Componente verde do pixel (10,10) da area do jogo: o cenario e 82; com o painel aberto fica escurecido.
int backdropGreen(HWND hwnd) {
    POINT o{10, 10};
    ClientToScreen(hwnd, &o);
    HDC screen = GetDC(nullptr);
    const COLORREF c = GetPixel(screen, o.x, o.y);
    ReleaseDC(nullptr, screen);
    return GetGValue(c);
}

// Teclas e cliques vao por mensagem direta para a janela do teste: nunca para outro programa.
void postKey(HWND hwnd, WORD vk) {
    PostMessageW(hwnd, WM_KEYDOWN, vk, 0);
    PostMessageW(hwnd, WM_KEYUP, vk, 0);
}

void sendKey(WORD vk, bool down) {
    INPUT in{};
    in.type = INPUT_KEYBOARD;
    in.ki.wVk = vk;
    in.ki.dwFlags = down ? 0 : KEYEVENTF_KEYUP;
    SendInput(1, &in, sizeof(in));
}

// Atalho de verdade (Ctrl + =), pelo teclado do Windows. So se a janela do teste estiver em primeiro plano:
// senao a entrada iria para outro programa.
bool sendComboCtrlEquals(HWND hwnd) {
    if (GetForegroundWindow() != hwnd) return false;
    sendKey(VK_CONTROL, true);
    sendKey(VK_OEM_PLUS, true);
    Sleep(120);
    sendKey(VK_OEM_PLUS, false);
    sendKey(VK_CONTROL, false);
    return true;
}

// Valor de um ajuste no ultimo status.json (NaN se nao achar).
double statusValue(const char *id) {
    aa::json::Value status;
    if (!readStatus(status)) return std::nan("");
    const aa::json::Value *f = findField(status, id);
    return f ? f->operator[]("value").asNumber(std::nan("")) : std::nan("");
}

bool closeTo(double a, double b) { return std::fabs(a - b) < 0.006; }

void expect(bool ok, const char *what, int &failures) {
    std::printf("[teste] %s %s\n", ok ? "OK:   " : "FALHA:", what);
    failures += ok ? 0 : 1;
}

// Gesto do mouse por mensagem direta, a partir do quadro 'start': aperta em (x0,y0), arrasta ate (x1,y1) e solta.
// Com a janela em foco o ImGui le a posicao do cursor real (GetCursorPos): leva o cursor junto e devolve no fim.
// Os botoes nunca vao para outro programa.
void driveMouse(HWND hwnd, int f, int start, int x0, int y0, int x1, int y1, POINT &saved) {
    if (f < start || f > start + 16) return;
    const bool second = f >= start + 10;
    const int x = second ? x1 : x0, y = second ? y1 : y0;
    if (f == start) GetCursorPos(&saved);
    if (f < start + 15) {
        POINT o{x, y};
        ClientToScreen(hwnd, &o);
        SetCursorPos(o.x, o.y);
    }
    if (f == start + 16) SetCursorPos(saved.x, saved.y);
    if (f == start + 3) PostMessageW(hwnd, WM_MOUSEMOVE, 0, MAKELPARAM(x0, y0));
    if (f == start + 8) PostMessageW(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(x0, y0));
    if (f == start + 11) PostMessageW(hwnd, WM_MOUSEMOVE, MK_LBUTTON, MAKELPARAM(x1, y1));
    if (f == start + 13) PostMessageW(hwnd, WM_LBUTTONUP, 0, MAKELPARAM(x1, y1));
}

int overlayTest(HMODULE dll, const std::wstring &outPath, bool mouse) {
    auto init = reinterpret_cast<InitFn>(GetProcAddress(dll, "scs_telemetry_init"));
    auto shutdown = reinterpret_cast<ShutdownFn>(GetProcAddress(dll, "scs_telemetry_shutdown"));
    if (!init || !shutdown) return 1;
    // O painel abre sozinho assim que o desvio instala: o teste nao depende de a janela ter o foco.
    SetEnvironmentVariableW(aa::ipc::kOverlayTestEnv, L"1");
    const scs_telemetry_init_params_v100_t params = makeParams();
    if (init(SCS_TELEMETRY_VERSION_1_01, &params) != SCS_RESULT_ok) return 1;
    fire(SCS_TELEMETRY_EVENT_started);

    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2); // como o jogo: coordenadas reais
    WNDCLASSEXW wc{sizeof(wc)};
    wc.lpfnWndProc = testWndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"FakeGameDx11";
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    RegisterClassExW(&wc);
    RECT r{0, 0, 1600, 900};
    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
    HWND hwnd = CreateWindowExW(0, wc.lpszClassName, L"Jogo falso DX11", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 60, 40,
                                r.right - r.left, r.bottom - r.top, nullptr, nullptr, wc.hInstance, nullptr);

    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 1600;
    sd.BufferDesc.Height = 900;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    IDXGISwapChain *sc = nullptr;
    ID3D11Device *dev = nullptr;
    ID3D11DeviceContext *ctx = nullptr;
    if (FAILED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0,
                                             D3D11_SDK_VERSION, &sd, &sc, &dev, nullptr, &ctx))) {
        std::printf("[teste] nao criou o dispositivo DX11\n");
        return 1;
    }
    ID3D11Texture2D *back = nullptr;
    ID3D11RenderTargetView *rtv = nullptr;
    sc->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void **>(&back));
    dev->CreateRenderTargetView(back, nullptr, &rtv);
    back->Release();

    int failures = 0;
    // Linha do tempo em quadros (~16 ms cada).
    const int kShot = 100, kKeys = 110, kTab = 130, kKeys2 = 140, kCheck = 180, kClick = 190, kCheckClick = 230,
              kDrag = 240, kCheckDrag = 280, kEsc = 290, kClosed = 330, kHotOpen = 340, kHotOpened = 380,
              kHotClose = 390, kEnd = 440;
    const int kRestoreX = 1004, kRestoreY = 323, kSliderY = 371, kSliderFromX = 720, kSliderToX = 900;
    const double volMinBefore = 0.35, airVolBefore = 0.90;
    double volMinAtStart = std::nan("");
    bool realHotkey = false;
    POINT savedCursor{0, 0};
    for (int f = 0; f <= kEnd; ++f) {
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        fire(SCS_TELEMETRY_EVENT_frame_end);

        const float bg[4] = {0.18f, 0.32f, 0.22f, 1.0f}; // "cenario" verde para ver o escurecimento
        ctx->OMSetRenderTargets(1, &rtv, nullptr);
        ctx->ClearRenderTargetView(rtv, bg);

        if (f == 5) {
            ShowWindow(hwnd, SW_SHOW);
            SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE); // a "foto" precisa da janela por cima
            SetForegroundWindow(hwnd); // o Windows pode negar (outro programa em uso): o teste segue sem o foco
        }
        if (f == 6) volMinAtStart = statusValue("brake.volume_min");
        if (f == kShot) {
            Sleep(100);
            std::printf("[teste] foto do painel: %s\n", saveScreenBmp(hwnd, outPath) ? "ok" : "FALHOU");
            expect(backdropGreen(hwnd) < 70, "painel abre por cima do quadro e escurece o jogo", failures);
        }
        // Teclado: aba Freio, linha 0 (volume minimo) +3 passos; Tab -> aba Saida de ar, linha 0 (volume) +1 passo.
        if (f == kKeys) {
            postKey(hwnd, VK_RIGHT);
            postKey(hwnd, VK_RIGHT);
            postKey(hwnd, VK_RIGHT);
        }
        if (f == kTab) postKey(hwnd, VK_TAB);
        if (f == kKeys2) postKey(hwnd, VK_RIGHT);
        if (f == kCheck) {
            Sleep(150);
            const double a = statusValue("brake.volume_min"), b = statusValue("air_release.volume");
            std::printf("[teste] volume_min %.3f (inicio %.3f), air_release.volume %.3f\n", a, volMinAtStart, b);
            expect(closeTo(volMinAtStart, volMinBefore) && closeTo(a, volMinBefore + 0.03),
                   "setas ajustam o controle escolhido (volume minimo +3%)", failures);
            expect(closeTo(b, airVolBefore + 0.01), "Tab troca de aba e as setas ajustam la (volume do ar +1%)", failures);
            saveScreenBmp(hwnd, outPath + L".teclado.bmp");
        }
        if (mouse) {
            // Coordenadas para a janela de 1600 x 900 (aba "Saida de ar"): botao "Restaurar padrao" e o controle "Volume".
            driveMouse(hwnd, f, kClick, kRestoreX, kRestoreY, kRestoreX, kRestoreY, savedCursor);
            driveMouse(hwnd, f, kDrag, kSliderFromX, kSliderY, kSliderToX, kSliderY, savedCursor);
        }
        if (mouse && f == kCheckClick) {
            Sleep(150);
            const double a = statusValue("brake.volume_min"), b = statusValue("air_release.volume");
            std::printf("[teste] depois do clique: volume_min %.3f, air_release.volume %.3f\n", a, b);
            expect(closeTo(a, volMinBefore) && closeTo(b, airVolBefore), "clique em \"Restaurar padrao\" volta os valores",
                   failures);
            saveScreenBmp(hwnd, outPath + L".depois.bmp");
        }
        if (mouse && f == kCheckDrag) {
            Sleep(150);
            const double b = statusValue("air_release.volume");
            std::printf("[teste] depois de arrastar o controle: air_release.volume %.3f (esperado ~0.52)\n", b);
            expect(std::fabs(b - 0.52) < 0.05, "arrastar o controle com o mouse muda o valor", failures);
            saveScreenBmp(hwnd, outPath + L".arrasto.bmp");
        }
        if (f == kEsc) postKey(hwnd, VK_ESCAPE);
        if (f == kClosed) expect(backdropGreen(hwnd) >= 78, "Esc fecha o painel e devolve o jogo", failures);
        // Atalho de verdade: so com a janela em primeiro plano (como no jogo).
        if (f == kHotOpen) realHotkey = sendComboCtrlEquals(hwnd);
        if (f == kHotOpened && realHotkey) expect(backdropGreen(hwnd) < 70, "Ctrl + = abre o painel", failures);
        if (f == kHotClose && realHotkey) sendComboCtrlEquals(hwnd);
        if (f == kEnd) {
            if (realHotkey) expect(backdropGreen(hwnd) >= 78, "Ctrl + = fecha o painel", failures);
            else std::printf("[teste] PULADO: Ctrl + = de verdade (a janela do teste nao ficou em primeiro plano)\n");
        }
        sc->Present(1, 0);
    }
    if (rtv) rtv->Release();
    if (ctx) ctx->Release();
    if (sc) sc->Release();
    if (dev) dev->Release();
    shutdown();
    return failures;
}

} // namespace

int main(int argc, char **argv) {
    const std::string mode = argc >= 3 ? argv[2] : "";
    if (mode != "--selftest" && !(mode == "--overlay" && argc >= 4)) {
        std::printf("Uso: fake_game <airbrake_audio.dll> --selftest\n"
                    "     fake_game <airbrake_audio.dll> --overlay <foto.bmp> [--mouse]\n");
        return 2;
    }
    const std::wstring dir = makeTempDir();
    SetEnvironmentVariableW(aa::ipc::kFolderOverrideEnv, dir.c_str()); // nao toca em Documentos

    HMODULE dll = LoadLibraryA(argv[1]);
    if (!dll) {
        std::printf("Nao consegui carregar %s (erro %lu)\n", argv[1], GetLastError());
        return 1;
    }
    if (mode == "--overlay") {
        const std::wstring bmp(argv[3], argv[3] + std::strlen(argv[3]));
        const bool mouse = argc >= 5 && std::string(argv[4]) == "--mouse";
        const int failures = overlayTest(dll, bmp, mouse);
        FreeLibrary(dll);
        std::printf("overlay_test: %s\n", failures == 0 ? "OK" : "FALHOU");
        if (failures) std::printf("--- log do jogo ---\n%s", g_log.c_str());
        return failures == 0 ? 0 : 1;
    }
    selftest(dll);
    FreeLibrary(dll);

    std::printf("%s\n", g_failures == 0 ? "plugin_selftest: OK" : "plugin_selftest: FALHOU");
    if (g_failures) std::printf("--- log do jogo ---\n%s", g_log.c_str());
    return g_failures == 0 ? 0 : 1;
}
