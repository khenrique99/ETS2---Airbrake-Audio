#pragma once
// Painel do Airbrake Audio desenhado DENTRO do jogo (DirectX 11 + Dear ImGui), como o do Trailer Control.
//
// Como funciona:
//  - Uma thread curta cria uma swap chain descartavel so para descobrir a vtable do IDXGISwapChain
//    e troca os ponteiros de Present / Present1 por funcoes nossas (o jogo usa a mesma vtable do
//    dxgi.dll). Nenhum codigo do exe do jogo e alterado e nada e lido da memoria do jogo.
//  - Com o painel aberto, cada Present desenha o ImGui por cima do quadro e a janela do jogo e
//    "subclassada" para o mouse/teclado irem so para o painel (o caminhao nao recebe nada).
//  - O estado vem de PanelHost::snapshot() a cada quadro e os comandos vao por PanelHost::submit().
//
// Threads: os hooks e o ImGui rodam na thread de render; toggle()/close() em qualquer uma (o atalho
// chega pela thread do teclado). Tudo que e compartilhado fica atras de mutex ou atomic.

#include "overlay/panel_host.h"

#include <atomic>
#include <thread>

namespace aa {

class Overlay {
public:
    // Vive ate o fim do processo (nunca destruido): uma thread std::thread juntada no descarregamento
    // da DLL pode travar sob o loader lock.
    static Overlay &instance();

    // Thread do jogo. enabled = [ui] mode = game. Instala os hooks na primeira vez.
    void configure(bool enabled, PanelHost *host);
    // Desfaz hooks e subclasse; antes de a DLL (e o host) sairem da memoria.
    void shutdown();

    // Hooks instalados e painel habilitado (senao o atalho abre o app externo).
    bool active() const { return hooked_.load() && enabled_.load() && !blockedByMultiplayer(); }
    bool isOpen() const { return open_.load(); }
    void toggle();
    void close();

    PanelHost *host() const { return host_.load(); }

private:
    Overlay() = default;
    static bool blockedByMultiplayer();

    std::atomic<bool> enabled_{false};
    std::atomic<bool> hooked_{false};
    std::atomic<bool> open_{false};
    std::atomic<bool> started_{false};
    std::atomic<PanelHost *> host_{nullptr};
    std::thread installThread_;
};

} // namespace aa
