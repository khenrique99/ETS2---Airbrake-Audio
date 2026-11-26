# Changelog

Todas as mudanças relevantes do projeto são documentadas aqui.
All notable changes to this project are documented here.

O formato segue o [Keep a Changelog](https://keepachangelog.com/pt-BR/) e o projeto usa
[Versionamento Semântico](https://semver.org/lang/pt-BR/). Versões anteriores à 1.0.0 podem
mudar sem aviso.

The format follows [Keep a Changelog](https://keepachangelog.com/) and the project uses
[Semantic Versioning](https://semver.org/). Versions before 1.0.0 may change without notice.

## [Unreleased]

## [2.1.0] - 2026-11-26

**Português:** o freio passa a ter dois sons (temperatura baixa e alta, conforme a força no pedal) e a saída de ar só toca depois de uma freada forte.

**English:** the brake now has two sounds (low and high temperature, by pedal force) and the air release only plays after a hard brake application.

### Adicionado / Added

- Dois sons de freio: com o pedal até **65%** toca o de **temperatura baixa** (`brake_loop_low.wav`); a partir disso, o de **temperatura alta** (`brake_loop.wav`), com troca cruzada pelos fades e histerese de 3 pontos. Limite ajustável na aba Freio (`brake.high_from`); a tela mostra qual dos dois está tocando.
  / Two brake sounds: with the pedal up to **65%** the **low temperature** one plays (`brake_loop_low.wav`); from there on, the **high temperature** one (`brake_loop.wav`), crossfaded through the fades with 3-point hysteresis. Threshold adjustable in the Brake tab (`brake.high_from`); the window shows which one is playing.

### Alterado / Changed

- A saída de ar só toca se a freada passou pelo menos **2 s** (somados) no som de temperatura alta (pedal acima de 65%); freada leve não solta ar. Ajustável na aba Saída de ar (`air_release.min_high_time_s`, 0 = comportamento antigo).
  / The air release only plays if the brake application spent at least **2 s** (summed) on the high temperature sound (pedal above 65%); light braking releases no air. Adjustable in the Air release tab (`air_release.min_high_time_s`, 0 = old behavior).

## [2.0.0] - 2026-11-25

**Português:** a tela de configuração deixa de ser uma página no navegador e vira o **AirbrakeAudio.exe**, com a mesma identidade visual do Trailer Control. O código do plugin foi dividido em componentes e ganhou testes.

**English:** the settings page is no longer a browser page: it is now **AirbrakeAudio.exe**, with the same look as Trailer Control. The plugin code was split into components and now has tests.

### Adicionado / Added

- App **AirbrakeAudio.exe** (Win32 + GDI+): métricas ao vivo, ajustes em abas (Freio, Saída de ar, Geral), salvar no `.ini` preservando comentários, restaurar padrão, português/inglês, sempre no topo e modo demonstração (`--demo`).
  / **AirbrakeAudio.exe** app (Win32 + GDI+): live metrics, tabbed settings, save to the `.ini` keeping comments, restore defaults, Portuguese/English, always on top and demo mode (`--demo`).
- Atalho **Ctrl + =** abre a tela no jogo (convive com o **Ctrl + -** do Trailer Control); trocável em Configurações.
  / **Ctrl + =** opens the window in game (coexists with the **Ctrl + -** of Trailer Control); changeable under Settings.
- Comunicação plugin ↔ tela por `status.json` e `command.txt` em `Documentos\ETS2AirbrakeAudio` (`src/shared/ipc.h`).
  / Plugin ↔ window communication via `status.json` and `command.txt` in `Documents\ETS2AirbrakeAudio`.
- Testes sem o jogo (`aa_tests`) e o "jogo falso" (`plugin_selftest`) que carrega a DLL real.
  / Tests without the game (`aa_tests`) and the "fake game" (`plugin_selftest`) that loads the real DLL.
- `scripts/build.ps1` (compila, testa, empacota), `scripts/publish-release.ps1` e `scripts/promote-to-main.ps1`.
  / `scripts/build.ps1` (build, test, package), `scripts/publish-release.ps1` and `scripts/promote-to-main.ps1`.
- Release com a versão do jogo no nome: tag `v2.0.0+ets2.1.62`, arquivo `AirbrakeAudio-v2.0.0-ETS2-1.62-plugins.zip`.
  / Releases carry the game version: tag `v2.0.0+ets2.1.62`, file `AirbrakeAudio-v2.0.0-ETS2-1.62-plugins.zip`.
- README em português e inglês separados e roteiros de teste no jogo (`docs/`).
  / Separate Portuguese and English READMEs and in-game test scripts (`docs/`).

### Alterado / Changed

- `[ui]` do `.ini`: `web_ui`/`port` saíram; agora `app=on|off` e `hotkey=CTRL+PLUS`. O `.ini` antigo continua lido (`web_ui`, `hotkey=Minus`), mas sem o servidor.
  / `[ui]` in the `.ini`: `web_ui`/`port` are gone; now `app=on|off` and `hotkey=CTRL+PLUS`. Old `.ini` files are still read (`web_ui`, `hotkey=Minus`), without the server.
- O plugin segue carregando sem placa de som ou com arquivo faltando: o motivo aparece na tela e no `game.log.txt`.
  / The plugin keeps loading without an audio device or with a missing file: the reason shows in the window and in `game.log.txt`.
- Licença: de MIT para **PolyForm Noncommercial 1.0.0** (copyright de Kelvin Henrique Ferreira): uso pessoal e não comercial liberado, venda e uso comercial não. As versões anteriores já distribuídas sob MIT continuam MIT.
  / License: from MIT to **PolyForm Noncommercial 1.0.0** (copyright Kelvin Henrique Ferreira): personal and non-commercial use is free, selling and commercial use are not. Earlier versions already distributed under MIT stay MIT.
- Mensagens de log usam o prefixo `[AirbrakeAudio]`.
  / Log messages use the `[AirbrakeAudio]` prefix.
- Código dividido em componentes (`brake_sounds`, `audio_engine`, `wav`, `config`, `app_bridge`...), compilado com `/W4 /WX`.
  / Code split into components, built with `/W4 /WX`.

### Removido / Removed

- Servidor HTTP local e página HTML embutida na DLL (`web.h`, `webpage.h`) e a dependência de `ws2_32`.
  / The local HTTP server and the HTML page embedded in the DLL, and the `ws2_32` dependency.
- `build.bat` (substituído por `scripts/build.ps1`).
  / `build.bat` (replaced by `scripts/build.ps1`).

## [1.0.0] - 2026-11-18

**Português:** primeira versão estável. Tela de configuração em HTML, aberta por uma tecla dentro do jogo (padrão: `-`).

**English:** first stable release. HTML settings page, opened with a key inside the game (default: `-`).

### Adicionado / Added

- Tela de configuração com métricas ao vivo e controles deslizantes (ganho, volumes, tempos), com
  botões para salvar no `.ini` e restaurar o padrão; português e inglês.
  / Settings page with live metrics and sliders (gain, volumes, timings), with buttons to save to the
  `.ini` and restore defaults; Portuguese and English.
- Tecla de atalho configurável (padrão `-`), na tela ou em `hotkey=`. Seção `[ui]` no `.ini`.
  / Configurable hotkey (default `-`), on the page or via `hotkey=`. New `[ui]` section in the `.ini`.
- Servidor local só em `127.0.0.1`, sob demanda e com desligamento por inatividade.
  / Local server on `127.0.0.1` only, on demand, shutting down when idle.
- O ganho (`boost_db`) agora pode ser mudado com o jogo rodando.
  / The gain (`boost_db`) can now be changed while the game is running.

## [0.1.0] - 2026-11-18

**Português:** plugin validado dentro do jogo. O som de freio padrão passou a ser o áudio do
mod de terceiro ETS Brake Fade (veja [CREDITS.md](CREDITS.md); não coberto pela MIT).

**English:** plugin validated in-game. The default brake sound is now the audio from the
third-party ETS Brake Fade mod (see [CREDITS.md](CREDITS.md); not covered by MIT).

### Alterado / Changed

- Som de freio padrão (`brake_loop.wav`) trocado pelo áudio do mod de terceiro.
  / Default brake sound (`brake_loop.wav`) replaced with the third-party mod's audio.
- Documentação atualizada: plugin validado no jogo e aviso de licença do som.
  / Documentation updated: plugin validated in-game and sound licensing notice.

## [0.0.1] - 2026-11-17

**Português:** primeira versão de pré-lançamento. Compila e carrega como plugin de
telemetria; ainda não validada dentro do jogo.

**English:** first preview release. Built and loaded as a telemetry plugin; not yet
validated in-game.

### Adicionado / Added

- Plugin de telemetria que lê o pedal de freio e a velocidade (somente leitura).
  / Telemetry plugin that reads the brake pedal and speed (read-only).
- Som de freio em loop com volume pelo pedal, fade de entrada/saída e silêncio abaixo de
  uma velocidade mínima.
  / Looping brake sound with pedal-driven volume, fade in/out and a minimum-speed mute.
- Saída de ar ao soltar o freio: som inteiro após freada longa; mais baixo e cortado cedo
  após freada curta.
  / Air release on brake release: full sound after a long application, quieter and cut
  early after a short one.
- Ganho com limitador suave (`boost_db`), independente do volume do jogo.
  / Gain stage with a soft limiter (`boost_db`), independent of the game volume.
- Configuração em `.ini` comentado, em português e inglês.
  / Commented `.ini` configuration, in Portuguese and English.
- Sons padrão, script de build MSVC, projeto CMake e documentação em português/inglês.
  / Default sounds, MSVC build script, CMake project and Portuguese/English documentation.

[2.1.0]: https://github.com/khenrique99/ETS2---Airbrake-Audio/releases/tag/v2.1.0%2Bets2.1.62
[2.0.0]: https://github.com/khenrique99/ETS2---Airbrake-Audio/releases/tag/v2.0.0%2Bets2.1.62
