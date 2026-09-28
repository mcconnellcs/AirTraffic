# Developer tools (optional)

The Arduino IDE route needs none of these commands. Use this page once the
basic build works, or if you want to contribute. Run project commands from
the extracted/cloned folder containing `README.md`.

On a Mac, open Terminal, type `cd ` (including the space), drag the project
folder from Finder into Terminal, and press Return. This handles spaces in
the path. `pwd` prints the current folder.

## Arduino CLI

Install [Arduino CLI](https://arduino.github.io/arduino-cli/latest/installation/)
first. These commands install the same versions as the IDE guide and CI:

```sh
arduino-cli config add board_manager.additional_urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core update-index
arduino-cli core install esp32:esp32@3.3.12
arduino-cli lib install 'LovyanGFX@1.2.30' 'ArduinoJson@7.4.3' 'WiFiManager@2.0.17'
arduino-cli compile --fqbn 'esp32:esp32:esp32s3:PSRAM=opi,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB' --output-dir build/BoardTest firmware/BoardTest
arduino-cli compile --fqbn 'esp32:esp32:esp32s3:PSRAM=opi,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB' --output-dir build/AirTraffic firmware/AirTraffic
```

The FQBN is Arduino's board/settings identifier. Unspecified settings use the
ESP32 core 3.3.12 defaults, including disabled USB CDC and Core 1 for Arduino's
loop. Compilation does not upload to the board. Use the
[IDE upload instructions](03-flash-it.md), or `arduino-cli board list` and
Arduino CLI's upload command with your actual port and matching FQBN.

## Host tests (no board needed)

On macOS, install Apple's command-line compiler tools if `c++ --version` or
`make --version` is unavailable:

```sh
xcode-select --install
```

Complete the installation dialog before continuing. Install **ArduinoJson
7.4.3** using Library Manager first. The default test path is
`~/Documents/Arduino/libraries/ArduinoJson/src`. If you changed the Arduino
sketchbook location, use the `src` folder inside that library instead.

```sh
make -C test
```

Or specify the library's actual path (quote it if it contains spaces):

```sh
make -C test ARDUINOJSON="$HOME/Documents/Arduino/libraries/ArduinoJson/src"
```

The test runners must both finish with `Status: SUCCESS!`. They cover pure
logic plus Wi-Fi setup using simulated APIs, not real hardware. Linux needs
Make and a C++17 compiler; set `ARDUINOJSON` to your installed library. Native
Windows command-line test setup is not covered here; GitHub Actions runs
these tests on Linux.

For optional coverage of the pure logic with Apple's Clang/LLVM tools:

```sh
make -C test coverage CXX=clang++
```

Coverage is not coverage of the whole firmware. Wi-Fi integration, display,
networking, and hardware behavior need separate checks. On Linux the coverage
target additionally needs matching `clang`, `llvm-profdata`, and `llvm-cov`.

## Python tools on a Mac

Run `python3 --version`; these pinned tools require **Python 3.9 or newer**.
Install a current Python 3 from [python.org](https://www.python.org/downloads/macos/)
if needed. Create a virtual environment once, so these tools do not depend on
or modify macOS's system Python:

```sh
python3 -m venv .venv
./.venv/bin/python -m pip install -r tools/requirements.txt
```

Use `./.venv/bin/python` in the commands below; no activation step is needed.

## Screenshots

With AirTraffic running, close Serial Monitor and any other program using the
port. Find the port with:

```sh
./.venv/bin/python -m serial.tools.list_ports
```

Replace the example port with yours:

```sh
./.venv/bin/python tools/screenshot.py /dev/cu.usbserial-XXXX radar.png
```

This saves `radar.png` in the current folder. Optionally add `--reset 1.5` to
restart and request a capture after 1.5 seconds; startup timing and Wi-Fi scans
can delay it. `shot` sends binary image data, so do not type it into Serial
Monitor expecting a readable picture. Image transfer briefly pauses rendering. The tool retries interrupted
captures; keep the board connected until it finishes.

## Fonts

The generated fonts are already committed. Only regenerate them when changing
fonts/sizes. After the Python setup above:

```sh
./.venv/bin/python tools/fonts/make_fonts.py
```

It rewrites `firmware/AirTraffic/fonts_data.cpp` and `fonts_data.h`. Review the
diff and upload to check that text still fits every screen. A new font name
also needs to be added to `theme::Font` and the matching `kFontData` array in
`ui_fonts.cpp`. Font licences are in `tools/fonts/`.

## Before sharing a change

Run `make -C test`, `python3 tools/check_docs.py`, and compile **both** sketches.
Then complete the relevant [hardware checks](validation.md). CI repeats the
document checks, host tests, and firmware compilation. It attaches firmware to
successful builds and publishes binaries for version tags; existing releases
are not updated when source files change.

[Back to README](../README.md)
