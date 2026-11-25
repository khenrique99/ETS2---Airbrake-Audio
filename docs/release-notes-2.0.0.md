**🇧🇷 Português** · [🇺🇸 English](#-english)

## Airbrake Audio 2.0.0 — ETS2 1.62.x

Som de freio e saída de ar, agora com uma **tela de ajustes** que abre com **Ctrl + =** dentro do jogo, no mesmo estilo do Trailer Control.

### Compatibilidade

| | |
|---|---|
| Jogo | Euro Truck Simulator 2 **1.62.x** (Steam, 64 bits) |
| Sistema | Windows 10/11 64 bits |
| Multiplayer | funciona (só toca áudio local; não altera nada do jogo) |

### Instalação

1. Baixe **`AirbrakeAudio-v2.0.0-ETS2-1.62-plugins.zip`** (abaixo, em *Assets*).
2. Extraia **dentro** de `...\steamapps\common\Euro Truck Simulator 2\bin\win_x64\plugins\` (crie a pasta `plugins` se não existir).
3. Abra o jogo e aceite o aviso de **"SDK features"**.
4. Dentro do jogo, **Ctrl + =** abre a tela. O atalho pode ser trocado em *Configurações*.

### Arquivos

| Arquivo | Para quê |
|---|---|
| `AirbrakeAudio-v2.0.0-ETS2-1.62-plugins.zip` | **tudo que vai na pasta `plugins`** (DLL, `.ini`, app e sons) |

Roteiro de teste no jogo: [docs/TESTE-NO-JOGO.md](https://github.com/khenrique99/ETS2---Airbrake-Audio/blob/main/docs/TESTE-NO-JOGO.md) · Problemas: abra uma [issue](https://github.com/khenrique99/ETS2---Airbrake-Audio/issues).

### Novidades

- Tela **AirbrakeAudio.exe** (substitui a página no navegador) com métricas ao vivo e ajustes em abas; salvar no `.ini` mantém os comentários.
- Atalho **Ctrl + =**, configurável; convive com o **Ctrl + -** do Trailer Control.
- Tela em português e inglês, modo demonstração e opção "sempre no topo".
- O plugin continua carregando sem placa de som; o motivo aparece na tela e no `game.log.txt`.
- Código reorganizado em componentes, com testes automáticos sem o jogo.
- **Licença:** agora [PolyForm Noncommercial 1.0.0](https://github.com/khenrique99/ETS2---Airbrake-Audio/blob/main/LICENSE): uso pessoal e não comercial liberado; **vender ou usar comercialmente não é permitido**.
- `[ui]` do `.ini` mudou: `app=on|off` e `hotkey=CTRL+PLUS` (`web_ui` e `port` saíram).

---

## 🇺🇸 English

## Airbrake Audio 2.0.0 — ETS2 1.62.x

Brake squeal and air release sound, now with a **settings window** that opens with **Ctrl + =** in game, in the same style as Trailer Control.

### Compatibility

| | |
|---|---|
| Game | Euro Truck Simulator 2 **1.62.x** (Steam, 64-bit) |
| System | Windows 10/11 64-bit |
| Multiplayer | works (it only plays local audio; it changes nothing in the game) |

### Install

1. Download **`AirbrakeAudio-v2.0.0-ETS2-1.62-plugins.zip`** (below, under *Assets*).
2. Extract it **into** `...\steamapps\common\Euro Truck Simulator 2\bin\win_x64\plugins\` (create the `plugins` folder if needed).
3. Start the game and accept the **"SDK features"** prompt.
4. In game, **Ctrl + =** opens the window. The key can be changed under *Settings*.

### Files

| File | Purpose |
|---|---|
| `AirbrakeAudio-v2.0.0-ETS2-1.62-plugins.zip` | **everything for the `plugins` folder** (DLL, `.ini`, app and sounds) |

In-game test script: [docs/TESTING-IN-GAME.md](https://github.com/khenrique99/ETS2---Airbrake-Audio/blob/main/docs/TESTING-IN-GAME.md) · Problems: open an [issue](https://github.com/khenrique99/ETS2---Airbrake-Audio/issues).

### What's new

- **AirbrakeAudio.exe** window (replaces the browser page) with live metrics and tabbed settings; saving to the `.ini` keeps its comments.
- **Ctrl + =** shortcut, configurable; coexists with Trailer Control's **Ctrl + -**.
- Window in Portuguese and English, demo mode and an "always on top" option.
- The plugin still loads without an audio device; the reason shows in the window and in `game.log.txt`.
- Code reorganized into components, with automated tests that run without the game.
- **License:** now [PolyForm Noncommercial 1.0.0](https://github.com/khenrique99/ETS2---Airbrake-Audio/blob/main/LICENSE): personal and non-commercial use is free; **selling it or using it commercially is not allowed**.
- `[ui]` in the `.ini` changed: `app=on|off` and `hotkey=CTRL+PLUS` (`web_ui` and `port` are gone).
