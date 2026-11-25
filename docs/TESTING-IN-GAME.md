# In-game test script (English)

Record each result and paste it in the PR. Game: ETS2 1.62.x, Steam, 64-bit.

## Setup

1. Extract the zip into `...\Euro Truck Simulator 2\bin\win_x64\plugins\`. You should get
   `airbrake_audio.dll`, `airbrake_audio.ini`, `AirbrakeAudio.exe` and an `airbrake_audio\` folder with the two `.wav` files.
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
| 6 | Press **Ctrl + =** in game | The Airbrake Audio window opens on top | |
| 7 | Drag "Maximum volume" while braking | Volume changes immediately | |
| 8 | Change **Gain** and release the control | Sounds stop for a moment, then return with the new gain | |
| 9 | **Save** and restart the game | Values kept, `.ini` comments preserved | |
| 10 | **Esc** or **Ctrl + =** in the window | Back to the game | |
| 11 | **Settings** → Change → another combination → Save | New shortcut applies immediately | |
| 12 | Exclusive fullscreen | Note whether the window shows or hides behind the game | |
| 13 | Coexists with Trailer Control (**Ctrl + -**) | Each shortcut opens only its own window | |

## Without the game

`AirbrakeAudio.exe --demo` simulates brake applications and answers commands like the plugin does.
The window text is in Brazilian Portuguese by default; the **EN** button switches it to English.
