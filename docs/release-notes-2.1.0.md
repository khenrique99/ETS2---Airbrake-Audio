**🇧🇷 Português** · [🇺🇸 English](#-english)

## Airbrake Audio 2.1.0 — ETS2 1.62.x

O freio agora tem **dois sons**, conforme a força no pedal, e a **saída de ar** só aparece depois de uma freada forte.

### Compatibilidade

| | |
|---|---|
| Jogo | Euro Truck Simulator 2 **1.62.x** (Steam, 64 bits) |
| Sistema | Windows 10/11 64 bits |
| Multiplayer | funciona (só toca áudio local; não altera nada do jogo) |

### Instalação

1. Baixe **`AirbrakeAudio-v2.1.0-ETS2-1.62-plugins.zip`** (abaixo, em *Assets*).
2. Extraia **dentro** de `...\steamapps\common\Euro Truck Simulator 2\bin\win_x64\plugins\` (crie a pasta `plugins` se não existir).
   Vindo da 2.0.0: **substitua todos os arquivos**, inclusive o `airbrake_audio.ini` e a pasta `airbrake_audio\` (tem um som novo).
3. Abra o jogo e aceite o aviso de **"SDK features"**.
4. Dentro do jogo, **Ctrl + =** abre a tela. O atalho pode ser trocado em *Configurações*.

### Arquivos

| Arquivo | Para quê |
|---|---|
| `AirbrakeAudio-v2.1.0-ETS2-1.62-plugins.zip` | **tudo que vai na pasta `plugins`** (DLL, `.ini`, app e os três sons) |

Roteiro de teste no jogo: [docs/TESTE-NO-JOGO.md](https://github.com/khenrique99/ETS2---Airbrake-Audio/blob/main/docs/TESTE-NO-JOGO.md) · Problemas: abra uma [issue](https://github.com/khenrique99/ETS2---Airbrake-Audio/issues).

### Novidades

- **Dois sons de freio:** com o pedal até **65%** toca o som de **temperatura baixa**; a partir de 65%, o de **temperatura alta**.
  A troca é cruzada (sem corte seco) e só volta ao som baixo quando o pedal desce abaixo de 62%, para não ficar trocando com o pedal tremendo.
  Pisar direto no fundo já começa no som alto.
- **Saída de ar só após freada forte:** ao soltar o freio, o ar só sai se a freada ficou pelo menos **2 s** (somados) acima de 65%.
  Freada leve não solta ar.
- Novos ajustes na tela: **"Som de temperatura alta a partir de"** (aba Freio) e **"Só toca após no som alto"** (aba Saída de ar).
  No `.ini`: `high_from`, `file_low` e `loop_start_low_s` em `[brake]`; `min_high_time_s` em `[air_release]` (0 = comportamento antigo).
- A tela mostra qual som está tocando: "freio · temp. baixa" ou "freio · temp. alta".

---

## 🇺🇸 English

## Airbrake Audio 2.1.0 — ETS2 1.62.x

The brake now has **two sounds**, depending on pedal force, and the **air release** only plays after a hard brake application.

### Compatibility

| | |
|---|---|
| Game | Euro Truck Simulator 2 **1.62.x** (Steam, 64-bit) |
| System | Windows 10/11 64-bit |
| Multiplayer | works (it only plays local audio; it changes nothing in the game) |

### Install

1. Download **`AirbrakeAudio-v2.1.0-ETS2-1.62-plugins.zip`** (below, under *Assets*).
2. Extract it **into** `...\steamapps\common\Euro Truck Simulator 2\bin\win_x64\plugins\` (create the `plugins` folder if needed).
   Coming from 2.0.0: **replace every file**, including `airbrake_audio.ini` and the `airbrake_audio\` folder (it has a new sound).
3. Start the game and accept the **"SDK features"** prompt.
4. In game, **Ctrl + =** opens the window. The key can be changed under *Settings*.

### Files

| File | Purpose |
|---|---|
| `AirbrakeAudio-v2.1.0-ETS2-1.62-plugins.zip` | **everything for the `plugins` folder** (DLL, `.ini`, app and the three sounds) |

In-game test script: [docs/TESTING-IN-GAME.md](https://github.com/khenrique99/ETS2---Airbrake-Audio/blob/main/docs/TESTING-IN-GAME.md) · Problems: open an [issue](https://github.com/khenrique99/ETS2---Airbrake-Audio/issues).

### What's new

- **Two brake sounds:** with the pedal up to **65%** the **low temperature** sound plays; from 65% on, the **high temperature** one.
  The swap crossfades (no hard cut) and only goes back to the low sound when the pedal drops below 62%, so a shaky pedal does not keep swapping.
  Stomping straight to the floor starts on the high sound.
- **Air release only after a hard brake:** on release, air only comes out if the brake application spent at least **2 s** (summed) above 65%.
  Light braking releases no air.
- New settings in the window: **"High-temperature sound from"** (Brake tab) and **"Only after high sound for"** (Air release tab).
  In the `.ini`: `high_from`, `file_low` and `loop_start_low_s` in `[brake]`; `min_high_time_s` in `[air_release]` (0 = old behavior).
- The window shows which sound is playing: "brake · low temp." or "brake · high temp.".
