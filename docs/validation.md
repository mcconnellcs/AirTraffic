# Validation and handoff checklist

This record applies to the source changes reviewed on **2026-09-28**, starting
from commit `abdc225` on `main`. It does not certify every board revision,
computer, network, or future dependency release.

## Software checks

- Arduino ESP32 core **3.3.12**; LovyanGFX **1.2.30**;
  ArduinoJson **7.4.3**; WiFiManager **2.0.17**.
- Both BoardTest and AirTraffic compile for the documented ESP32-S3 settings.
- **82 host test cases** pass across the pure-logic and Wi-Fi-controller runners.
  Wi-Fi controller tests use simulated APIs; they do not emulate the radio.
- Local Markdown file links and documented dependency versions are checked by
  `tools/check_docs.py`, also run in CI.
- Both flight providers returned aircraft JSON when requested from this Mac.
  Route lookup, IP location, and an airline-logo download also responded.
  These spot checks do not guarantee future service availability.

## Corrections made in this review

- Exact dependency versions and Mac install/port/USB settings replace broad
  version ranges and unspecified defaults.
- The build guide now explains ZIP extraction, complete sketch folders,
  verify versus upload, upload recovery that never needs the BOOT/RST buttons
  (they are inside the enclosure), and success checkpoints.
- Serial commands explicitly require a newline. Ready-made firmware instructions
  use esptool 5 syntax in a Python virtual environment and explain image offsets
  and settings erasure.
- Wi-Fi setup stays visible while an old network remains connected, opens with
  current screen settings, and preserves those settings when saving the form.
  Opening setup from demo mode switches back to live mode.
- Invalid coordinates including whitespace-only input and non-finite numbers
  are rejected instead of becoming an invalid manual location.
- Both sketches stop with a serial diagnostic if required PSRAM/display startup
  fails, instead of continuing to draw with unavailable resources.
- Screenshots are encoded into one UART write so background download logs
  cannot be inserted between image packets. Hardware testing exposed the
  previous corruption; regression tests cover run lengths and byte order.
- Documentation now describes approximate location, missing data, simulated
  flights, actual cache behavior, and the clock's time-zone/DST limitations.

## Physical board checks

On the owner's connected ESP32-S3 board, using this Mac and a USB serial port:

- BoardTest uploaded at 460800 baud and verified its flash writes.
- The board reported **16,777,216 bytes flash** and **8,384,788 bytes free PSRAM**
  before display setup. BoardTest ran at about **19 FPS**.
- The owner confirmed all four colours looked correct and the orange dot
  followed centre/corner touches. Serial captured touch coordinates across
  the glass.
- The updated AirTraffic firmware uploaded and reached its setup screen at
  about **30 FPS**. The USB screenshot tool captured a complete 480 × 480 image.
- The owner completed the phone setup flow and the board reached live radar.
  Serial confirmed a live `adsb.lol` feed with nearby aircraft, and a captured
  screenshot showed the radar, time, labels, and LIVE indicator.
- The first phone attempt returned to setup; it succeeded with a stable serial
  monitoring connection. Opening the serial port was observed to reset this
  board, so that may have interrupted the earlier attempt; the cause was not
  proven. The guide now explains this behavior.
- A software restart reconnected to the saved Wi-Fi and resumed live aircraft
  downloads without re-entering credentials.
- Reopening Wi-Fi setup while online kept the setup screen visible. Its HTTP
  form showed the current metric setting. A form save completed successfully.
- After saving the setup form, a screenshot confirmed **metric**, **24 HOUR**,
  and the existing **10 NM** range were preserved. The original aviation units
  and 12-hour format were then restored.
- With the screenshot transfer fix, consecutive settings-screen captures
  completed without corrupt packets while live downloads continued.
- Serial-driven gestures opened the nearby-flight list and a flight card,
  then returned to the radar. Complete screenshots were inspected for both.
- Demo mode showed all **14 simulated aircraft** at **50 NM**. Demo was then
  turned off, the original **10 NM**, aviation-unit, 12-hour settings restored,
  and serial confirmed fresh live aircraft data from `adsb.lol`.
- No unexpected reset or panic was observed during the monitored sessions.
  Intentional uploads and software resets were used to test reconnects.

Additional developer checks: a fresh source copy with spaces in its folder
name passed the host tests and compiled AirTraffic. The optional Python tools
installed in a virtual environment; Pillow 11.3.0 reproduced the committed
font data exactly. esptool 5.4.0 installed and its command syntax was checked
under Python 3.12. The default Python 3.9 was too old for esptool 5; the guide
now calls this out. Coverage tooling also ran successfully. Its report covers
selected pure-logic files, not the whole firmware.

## Independent re-check (2026-09-28)

A second pass repeated the beginner's route from a fresh **Download ZIP** of
`main` on the same Mac and board, using Arduino CLI with the documented settings:

- Every option name and value in the [Tools settings table](02-install-arduino.md#step-5--set-the-board-options)
  was compared with the ESP32 core 3.3.12 definition of **ESP32S3 Dev Module**;
  all match, including the listed **460800** upload speed.
- Both sketches compiled from the unzipped `AirTraffic-main` folder and uploaded
  at 460800 baud. BoardTest reported 16,777,216 bytes flash, 8,384,788 bytes
  free PSRAM, `Touch driver: OK`, and about 19 FPS.
- After uploading AirTraffic, the saved Wi-Fi network was rejoined and the first
  flights arrived within about 35 seconds. `status`, `help`, and an unknown
  command produced the replies the guides describe. Saved settings survived.
- The ready-made-firmware commands were run with esptool 5.4.0 under Python
  3.12 (`write-flash`, port listing, and a read-only chip check of this board).
  Installing esptool 5.4.0 under the Mac's bundled Python 3.9 failed, as the
  guide warns. The optional tools in `tools/requirements.txt` installed under
  Python 3.9.
- Opening the serial port from a script did **not** restart the board on this
  Mac, while the earlier session saw restarts. Guide 4 now says to wait for the
  radar if the start-up animation plays when Serial Monitor opens.
- The README screenshots were recaptured in demo mode so they show simulated
  flights rather than a real location.

Arduino IDE itself was not installed on the reviewing Mac, so its windows and
installation dialogs were not clicked through. The settings it displays come
from the same core definition that was checked above.

## Checklist for the beginner's own build

Repeat these checks on his Mac and board. A fresh Arduino IDE installation on
his Mac, a physical unplug/replug, and an uninterrupted 30-minute run remain
unverified by this review.

- [ ] Have the same model board, a known data cable/adapter, and Wi-Fi password ready.
- [ ] Confirm the beginner's Mac can install Arduino IDE and the exact dependencies.
- [ ] Follow guides 1–4 from a fresh ZIP, without relying on files from this Mac.
- [ ] Run BoardTest: verify four colours, moving line, and touch across the glass.
- [ ] Upload AirTraffic: check the setup page on a phone and join 2.4 GHz Wi-Fi.
- [ ] Confirm the radar centre; test manual coordinates if automatic location is wrong.
- [ ] Open/close a card, scroll the list, cycle RANGE, and change units/clock.
- [ ] Reopen Wi-Fi setup while connected; save and check screen settings persist.
- [ ] Unplug/replug and confirm saved Wi-Fi reconnects.
- [ ] Leave it running for at least 30 minutes; check for resets or display corruption.

A software build and automated UI commands cannot prove a novice can complete
every installation dialog or that physical touch and power behave correctly.
Use these checks to close those gaps rather than promising zero possible issues.

[Start the build](01-what-you-need.md) · [Back to README](../README.md)
