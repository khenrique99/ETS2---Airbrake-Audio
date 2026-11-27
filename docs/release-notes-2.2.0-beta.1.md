**🇧🇷 Português** · [🇺🇸 English](#-english)

## Airbrake Audio 2.2.0-beta.1 — ETS2 1.62.x — BETA DE TESTE

> 🧪 **Beta de teste do painel dentro do jogo.** Foi testado **só com um "jogo falso" em DirectX 11**, nunca no ETS2 de verdade. Para uso normal, prefira a versão estável [2.1.0](https://github.com/khenrique99/ETS2---Airbrake-Audio/releases/tag/v2.1.0%2Bets2.1.62). Salve o jogo antes de usar.

### O que muda

- **Painel dentro do jogo**: **Ctrl + =** abre o painel **por cima da imagem do ETS2**, sem tirar o jogo de primeiro plano (sem alt-tab). Mesmo visual e funções do app externo: pedal, velocidade, tempo de freada, som tocando, ajustes por aba, Salvar, Restaurar padrão e PT/EN.
- Com o painel aberto, mouse e teclado vão só para ele e o jogo continua rodando atrás (dá para frear e ouvir o ajuste ao vivo). **Esc**, **Fechar** ou o atalho fecham. Sem mouse: **↑ ↓** escolhem o ajuste, **← →** mudam o valor, **Tab** troca de aba, **Ctrl + S** salva.
- Precisa do ETS2 em **DirectX 11** (o padrão). Se o plugin não conseguir se pendurar no vídeo, abre o `AirbrakeAudio.exe` sozinho. `[ui] mode = external` no `.ini` força o app externo. No TruckersMP o painel fica desligado.
- O plugin continua **só lendo** pedal e velocidade pelo Telemetry SDK: o painel desvia o `Present` do DirectX 11 (`dxgi.dll`), sem alterar o `eurotrucks2.exe` nem ler a memória do jogo.

### Instalação

1. Baixe **`AirbrakeAudio-v2.2.0-ETS2-1.62-beta-plugins.zip`** e extraia **dentro** de `...\steamapps\common\Euro Truck Simulator 2\bin\win_x64\plugins\` (**substitua todos os arquivos**, inclusive o `airbrake_audio.ini`, que ganhou `[ui] mode`).
2. Abra o jogo e aceite o aviso de **"SDK features"**.
3. No jogo, **Ctrl + =** abre o painel.

### Como ajudar a testar

Roteiro: [docs/TESTE-NO-JOGO.md](https://github.com/khenrique99/ETS2---Airbrake-Audio/blob/dev/docs/TESTE-NO-JOGO.md) (linhas 22 a 26 são do painel). Se o jogo travar ao abrir o painel, o mouse/teclado se comportarem de forma estranha ou o painel não aparecer, abra uma [issue](https://github.com/khenrique99/ETS2---Airbrake-Audio/issues) com as linhas `[AirbrakeAudio]` do `game.log.txt` (procure "Painel no jogo") e use `[ui] mode = external` enquanto isso.

---

## 🇺🇸 English

## Airbrake Audio 2.2.0-beta.1 — ETS2 1.62.x — TEST BETA

> 🧪 **Test beta of the in-game panel.** It was tested **only with a "fake game" in DirectX 11**, never in the real ETS2. For normal use, prefer the stable [2.1.0](https://github.com/khenrique99/ETS2---Airbrake-Audio/releases/tag/v2.1.0%2Bets2.1.62). Save your game first.

### What changes

- **In-game panel**: **Ctrl + =** opens the panel **over the ETS2 picture**, without taking the game out of the foreground (no alt-tab). Same look and features as the external app: pedal, speed, brake time, sound playing, settings by tab, Save, Restore defaults and PT/EN.
- While the panel is open the mouse and keyboard go only to it and the game keeps running behind it (you can brake and hear the change live). **Esc**, **Close** or the shortcut closes it. No mouse: **↑ ↓** pick a setting, **← →** change the value, **Tab** switches tab, **Ctrl + S** saves.
- Needs ETS2 on **DirectX 11** (the default). If the plugin cannot hook the video it opens `AirbrakeAudio.exe` by itself. `[ui] mode = external` in the `.ini` forces the external app. The panel is off on TruckersMP.
- The plugin still **only reads** the pedal and speed through the Telemetry SDK: the panel detours DirectX 11's `Present` (`dxgi.dll`), without modifying `eurotrucks2.exe` or reading game memory.

### Install

1. Download **`AirbrakeAudio-v2.2.0-ETS2-1.62-beta-plugins.zip`** and extract it **into** `...\steamapps\common\Euro Truck Simulator 2\bin\win_x64\plugins\` (**replace every file**, including `airbrake_audio.ini`, which gained `[ui] mode`).
2. Start the game and accept the **"SDK features"** prompt.
3. In game, **Ctrl + =** opens the panel.

### Helping test

Checklist: [docs/TESTING-IN-GAME.md](https://github.com/khenrique99/ETS2---Airbrake-Audio/blob/dev/docs/TESTING-IN-GAME.md) (rows 22–26 cover the panel). If the game freezes when the panel opens, the mouse/keyboard misbehave or the panel does not show, open an [issue](https://github.com/khenrique99/ETS2---Airbrake-Audio/issues) with the `[AirbrakeAudio]` lines from `game.log.txt` (look for "Painel no jogo") and use `[ui] mode = external` meanwhile.
