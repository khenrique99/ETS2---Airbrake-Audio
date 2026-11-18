# Changelog

Todas as mudanças relevantes do projeto são documentadas aqui.
All notable changes to this project are documented here.

O formato segue o [Keep a Changelog](https://keepachangelog.com/pt-BR/) e o projeto usa
[Versionamento Semântico](https://semver.org/lang/pt-BR/). Versões anteriores à 1.0.0 podem
mudar sem aviso.

The format follows [Keep a Changelog](https://keepachangelog.com/) and the project uses
[Semantic Versioning](https://semver.org/). Versions before 1.0.0 may change without notice.

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

[1.0.0]: https://github.com/khenrique99/ETS2---Airbrake-Audio/releases/tag/v1.0.0
[0.1.0]: https://github.com/khenrique99/ETS2---Airbrake-Audio/releases/tag/v0.1.0
[0.0.1]: https://github.com/khenrique99/ETS2---Airbrake-Audio/releases/tag/v0.0.1
