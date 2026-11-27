# In-game test script (English)

Record each result and paste it in the PR. Game: ETS2 1.62.x, Steam, 64-bit.

## Setup

1. Extract the zip into `...\Euro Truck Simulator 2\bin\win_x64\plugins\`. You should get
   `airbrake_audio.dll`, `airbrake_audio.ini`, `AirbrakeAudio.exe` and an `airbrake_audio\` folder with the three `.wav` files.
2. Start the game with `-nointro -developer` and accept the **"SDK features"** prompt.
3. Open `Documents\Euro Truck Simulator 2\game.log.txt` and look for `[AirbrakeAudio] v... pronto`.
   If you see `Sem som: ...`, the reason is on the same line (missing file, format, no device).

## Items

| # | Do | Expected | OK? |
|---|---|---|---|
| 1 | Drive at ~50 km/h and brake hard | Looping brake squeal, volume follows the pedal | |
| 2 | Release after ~6 s | Full air release at full volume | |
| 3 | Brake for only ~0.6 s and release | Quieter air release, cut early | |
| 4 | Brake while stopped (< 3 km/h) | No squeal | |
| 5 | Pause with the brake pressed | Instant silence; normal again after resuming | |
| 6 | Press **Ctrl + =** in game | The panel opens **inside the game**, over the image, and the game dims behind it | |
| 7 | In the panel, drag "Maximum volume" while braking | Volume changes immediately | |
| 8 | Change **Gain** and release the control | Sounds stop for a moment, then return with the new gain | |
| 9 | **Save** and restart the game | Values kept, `.ini` comments preserved | |
| 10 | **Esc**, **Close** and **Ctrl + =** (once each) | The panel closes; the mouse turns the camera again; no key stays "stuck" | |
| 11 | Change `hotkey` under `[ui]` in the `.ini`, restart the game | The new shortcut opens the panel | |
| 12 | Open the panel with the game in **fullscreen**, **windowed** and **borderless**; change the resolution | Shows in all three; its size follows the resolution | |
| 13 | Coexists with Trailer Control (**Ctrl + -**) | Each shortcut opens only its own panel | |
| 14 | Brake with the pedal up to ~50% | **Low temperature** sound; window shows "brake · low temp." | |
| 15 | Press the pedal past 65% | Swaps to the **high temperature** sound with no hard cut; window shows "brake · high temp." | |
| 16 | Ease the pedal back to ~50% without releasing | Back to the low temperature sound, smooth crossfade | |
| 17 | Stomp the pedal straight to the floor | Starts directly on the high temperature sound | |
| 18 | In the window, change "High-temperature sound from" | The swap point changes immediately | |
| 19 | Brake ~6 s only below 65% and release | **No** air release | |
| 20 | Brake above 65% for ~1 s and release | **No** air release | |
| 21 | Brake above 65% for over 2 s and release | Air release plays | |
| 22 | With the panel open, move the mouse and press game keys (W, A, S, D, the game's arrows) | The panel cursor follows the mouse; the truck and camera do not react | |
| 23 | Panel open: **↑ ↓** pick a setting, **← →** change the value, **Tab** switches tab, **Ctrl + S** saves | Same as the mouse; the picked setting is highlighted | |
| 24 | With the panel open, brake and listen while changing "Maximum volume" and "High-temperature sound from" | The sound changes live; the panel shows pedal, speed and the sound playing | |
| 25 | **EN/PT** button in the panel; close and reopen | The language is remembered (it also applies to `AirbrakeAudio.exe`) | |
| 26 | `mode = external` under `[ui]` in the `.ini`; restart and press **Ctrl + =** | The separate `AirbrakeAudio.exe` window opens, as before | |

> **In-game panel (beta):** it was only tested with a "fake game" on DirectX 11 (`fake_game --overlay`), not in ETS2.
> If the game crashes when the panel opens, or the mouse/keyboard behave strangely, note what happened, send
> `game.log.txt` (look for "Painel no jogo") and use `[ui] mode = external` until it is fixed.

## Without the game

`AirbrakeAudio.exe --demo` simulates brake applications and answers commands like the plugin does.
The window text is in Brazilian Portuguese by default; the **EN** button switches it to English.
