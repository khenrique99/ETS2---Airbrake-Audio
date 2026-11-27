# ETS2 Airbrake Audio

[🇧🇷 Português](README.md) · **🇺🇸 English**

A **sound** plugin for **Euro Truck Simulator 2** (and ATS) with an **in-game panel** to tune everything
(and **AirbrakeAudio.exe**, the same screen in a separate window). It plays a glazed-lining brake squeal while the pedal is pressed and an air release when it is let go.

| | |
|---|---|
| **Version** | 2.2.0-beta.1 (in-game panel beta; stable: 2.1.0) |
| **Game** | ETS2 **1.62.x** (Steam, 64-bit); it only reads the pedal and speed through the Telemetry SDK |
| **System** | Windows 10/11 64-bit |
| **Mode** | single-player and multiplayer (it only plays local audio, it changes nothing in the game) |

## Download and install

1. Download **`AirbrakeAudio-...-plugins.zip`** from the
   [Releases](https://github.com/khenrique99/ETS2---Airbrake-Audio/releases) page. The release name carries the game
   version (`v2.0.0+ets2.1.62` = plugin 2.0.0 built for ETS2 1.62.x).
2. Extract it **into the game's `plugins` folder**:
   `...\steamapps\common\Euro Truck Simulator 2\bin\win_x64\plugins\` (create it if needed). You should get:

   ```
   plugins\airbrake_audio.dll
   plugins\airbrake_audio.ini
   plugins\AirbrakeAudio.exe
   plugins\airbrake_audio\brake_loop.wav
   plugins\airbrake_audio\brake_loop_low.wav
   plugins\airbrake_audio\air_release.wav
   ```

3. Start the game and accept the **"SDK features"** prompt.
4. Check `game.log.txt` (`Documents\Euro Truck Simulator 2`) for `[AirbrakeAudio] v... pronto`. On failure the same line
   says why (sound file not found, unsupported format, no audio device).

## Usage

- **Ctrl + =** in game opens the panel **over the game image**, without taking the game out of the foreground.
  It needs ETS2 on **DirectX 11** (the default). While it is open, mouse and keyboard go only to the panel and the
  game keeps running behind it (you can brake and hear the change). **Esc**, **Close** or the same shortcut closes it.
- The panel shows the pedal, speed, brake time and the sound playing, live.
- **Settings** (Brake, Air release, General tabs) apply immediately; **Save** writes `airbrake_audio.ini`
  keeping its comments. **Restore defaults** resets everything (and only writes if you save).
- Without a mouse: **↑ ↓** pick a setting, **← →** change its value, **Tab** switches tab and **Ctrl + S** saves.
- **PT/EN** switches the panel language.
- The shortcut that opens the panel is `[ui] hotkey` in `airbrake_audio.ini`. `[ui] mode = external` goes back to the
  separate **AirbrakeAudio.exe** window (with **Settings** to change the shortcut and **Demo**, which simulates brake
  applications without the game running). If the in-game panel cannot install (game not on DirectX 11), the plugin
  opens `AirbrakeAudio.exe` by itself. On **TruckersMP** the in-game panel is turned off.
- The default shortcut (**Ctrl + =**) coexists with [Trailer Control](https://github.com/khenrique99/ETS2---Trailer-Control) (**Ctrl + -**).

To use your own sounds, put 16-bit PCM WAV files (mono or stereo) in `plugins\airbrake_audio` and set `file=` in
`[brake]` (`file=` high temperature, `file_low=` low temperature) and `file=` in `[air_release]`. For the brake loops,
set `loop_start_s` / `loop_start_low_s` (applies after restarting the game).

The brake has two sounds: with the pedal up to **65%** the **low temperature** one plays; from there on, the
**high temperature** one. The swap crossfades with the fades (no hard cut) and the threshold is adjustable in the Brake
tab ("High-temperature sound from", `high_from` in the `.ini`). The **air release** only plays on release if the
brake application spent at least **2 s** on the high temperature sound (`min_high_time_s` in `[air_release]`, 0 = always).

## It does not touch game mechanics

The plugin only **reads** the brake pedal and speed through the official SCS Telemetry SDK and plays audio. It does not
change brake temperature, braking force or physics, and it does not read or patch game memory. The in-game panel draws
over the frame by detouring DirectX 11's `Present` (the function table of `dxgi.dll`, inside the game process); no
`eurotrucks2.exe` code is modified. It coexists with
brake-heating plugins. If another plugin also plays brake sounds, both play at once: turn the sound off in that plugin's
own configuration instead of deleting its files.

## For developers

### Build

Requires Visual Studio 2022 or Build Tools (Desktop C++); CMake ships with it. **No SCS SDK download needed**
(`src/scs_sdk_min.h` declares only what is used). The panel's [Dear ImGui](https://github.com/ocornut/imgui) (MIT,
v1.91.9) is downloaded by CMake on the first configure (needs internet and `git`); it is not in the repository.

```powershell
powershell -ExecutionPolicy Bypass -File scripts\build.ps1
```

It builds, runs the tests and writes the release zip to `dist\`. The name carries the plugin version (`src/version.h`)
and the game version (`-GameTag ETS2-1.62`, the default). `-SkipTests` skips the tests.

### Tests (no game)

- `aa_tests` — WAV, configuration, brake logic (with a fake audio output), commands, shortcuts, JSON and the window model.
- `plugin_selftest` — the **fake game** (`tests/fake_game.cpp`) loads the real DLL through the SDK, simulates the pedal
  and sends commands like the window would, checking `status.json`.
- `fake_game --overlay` (manual, needs a display): opens a real DirectX 11 window, opens the panel in it, takes
  "photos", adjusts controls by keyboard and (with `--mouse`) by click and drag, and checks `status.json`:
  `build\Release\fake_game.exe build\Release\airbrake_audio.dll --overlay photo.bmp --mouse`.

In-game script: [docs/TESTING-IN-GAME.md](docs/TESTING-IN-GAME.md).

### Plugin ↔ window protocol

Files in `Documents\ETS2AirbrakeAudio\`: `status.json` (plugin → window) and `command.txt` (window → plugin).
Protocol in [src/shared/ipc.h](src/shared/ipc.h).

## Contributing

Commits follow [Conventional Commits](https://www.conventionalcommits.org/) and versions follow
[Semantic Versioning](https://semver.org/); see [CHANGELOG.md](CHANGELOG.md).

## License

Code: © 2026 Kelvin Henrique Ferreira, under the [PolyForm Noncommercial 1.0.0](LICENSE): personal and non-commercial use is free; **selling it or using it commercially is not allowed** without the author's permission. Sounds: see [CREDITS.md](CREDITS.md) (the brake sound is third-party and **not** covered by this license).
Fan project, not affiliated with SCS Software. Euro Truck Simulator 2 is a trademark of SCS Software.
