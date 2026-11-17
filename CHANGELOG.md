# Changelog

Todas as mudanças relevantes do projeto são documentadas aqui.
All notable changes to this project are documented here.

O formato segue o [Keep a Changelog](https://keepachangelog.com/pt-BR/) e o projeto usa
[Versionamento Semântico](https://semver.org/lang/pt-BR/). Versões anteriores à 1.0.0 podem
mudar sem aviso.

The format follows [Keep a Changelog](https://keepachangelog.com/) and the project uses
[Semantic Versioning](https://semver.org/). Versions before 1.0.0 may change without notice.

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

[0.0.1]: https://github.com/khenrique99/ETS2---Airbrake-Audio/releases/tag/v0.0.1
