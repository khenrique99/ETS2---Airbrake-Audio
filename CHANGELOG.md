# Changelog

All notable changes to this project are documented here. / Todas as mudanças relevantes do
projeto são documentadas aqui.

The format follows [Keep a Changelog](https://keepachangelog.com/) and the project uses
[Semantic Versioning](https://semver.org/). Versions before 1.0.0 may change without notice.

## [0.0.1] - 2026-11-17

First preview release. Built and loaded as a telemetry plugin; not yet validated in-game. /
Primeira versão de pré-lançamento. Compila e carrega como plugin de telemetria; ainda não
validada dentro do jogo.

### Added / Adicionado

- Telemetry plugin that reads the brake pedal and speed (read-only). /
  Plugin de telemetria que lê o pedal de freio e a velocidade (somente leitura).
- Looping brake sound with pedal-driven volume, fade in/out and a minimum-speed mute. /
  Som de freio em loop com volume pelo pedal, fade de entrada/saída e silêncio abaixo de
  uma velocidade mínima.
- Air release on brake release: full sound after a long application, quieter and cut early
  after a short one. /
  Saída de ar ao soltar o freio: som inteiro após freada longa; mais baixo e cortado cedo
  após freada curta.
- Gain stage with a soft limiter (`boost_db`), independent of the game volume. /
  Ganho com limitador suave (`boost_db`), independente do volume do jogo.
- Commented `.ini` configuration. / Configuração em `.ini` comentado.
- Default sounds, MSVC build script, CMake project and English/Portuguese documentation. /
  Sons padrão, script de build MSVC, projeto CMake e documentação em inglês/português.

[0.0.1]: https://github.com/khenrique99/ETS2---Airbrake-Audio/releases/tag/v0.0.1
