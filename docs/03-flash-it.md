# 3. Flash it

*Compile* means turning code into a program. *Upload* (or *flash*) means
copying that program to the board. Upload replaces the factory demo.

## Step 1 — Download and unzip the whole project

1. Open [AirTraffic on GitHub](https://github.com/mcconnellcs/AirTraffic).
2. Choose the green **Code ▸ Download ZIP** button.
3. On the Mac, double-click the downloaded ZIP in Finder to extract it if it
   has not already expanded. Move the resulting `AirTraffic-main` folder to
   somewhere easy to find, such as Documents.
4. Open that folder. You should see `README.md`, `docs`, `firmware`, `test`,
   and `tools`.

Keep the whole folder together. Do not copy just the `.ino` file into a new
sketch: it needs the adjacent `.h` and `.cpp` files. You do not need a GitHub
account or Git. If you already use Git, cloning the repository also works.

## Step 2 — Upload BoardTest

1. In Arduino IDE choose **File ▸ Open…**, navigate into the extracted folder,
   and open **firmware ▸ BoardTest ▸ BoardTest.ino**.
2. Recheck the [Tools settings](02-install-arduino.md#step-5--set-the-board-options),
   including the port and **OPI PSRAM**.
3. Click the **✓ Verify** button at the top left. Wait for compilation to
   finish. If it fails, fix the **first error** using
   [Troubleshooting](troubleshooting.md); do not proceed to AirTraffic yet.
4. Click **→ Upload**. Keep the board connected until the IDE reports that
   uploading completed. `Writing at…`, verification messages, and
   `Hard resetting via RTS pin…` are normal.
5. Watch the board. You should see red, green, blue, and white bars for about
   **3 seconds**, then a spinning green line and an FPS number.
6. Touch several parts of the screen. An orange dot should follow your finger.

If you missed the bars, press and release **RST** to restart. BoardTest uses
the display's native orientation, so its text may face a different direction
from AirTraffic. That alone is not a failure.

Open **Tools ▸ Serial Monitor**, select **115200 baud**, and press **RST**
again to see the report. Flash should be **16777216 bytes** (16 MB), PSRAM
should be nonzero, and touching should print `Touch at x,y`. The message
`Touch driver: OK` alone does not prove touch works; actually touch the glass.

**Checkpoint:** all four colours look right, the line moves, and the dot
tracks touches across the screen. If any check fails, stop here and use
[Troubleshooting](troubleshooting.md#the-screen).

### If upload stops at “Connecting…”

1. Close Serial Monitor and any other program using the board's port.
2. Hold **BOOT**, press and release **RST**, then release **BOOT**.
3. Click **Upload** again. If necessary set Upload Speed to **115200**.
4. After a successful manual upload, press and release **RST** with BOOT
   released so the program starts.

This button sequence puts the chip into its download mode; it does not erase
settings by itself. Button labels can be **EN** instead of RST or **IO0**
instead of BOOT. If your enclosure hides them, disconnect USB before opening it.

## Step 3 — Upload AirTraffic

1. Choose **File ▸ Open…** and open
   **firmware ▸ AirTraffic ▸ AirTraffic.ino** in the same extracted project.
2. Check the Tools settings **in this sketch window**. In particular, keep
   the 16 MB flash, 3 MB app partition, OPI PSRAM, and correct serial port.
3. Click **✓ Verify**, then **→ Upload**. The first build can take several
   minutes. Do not unplug the board until upload finishes.
4. The board restarts into the AirTraffic animation. On a fresh board it
   should show **Let's get connected** and a QR code. If it already remembers
   a working Wi-Fi network, it can go directly to the radar.

No code edits, Wi-Fi password file, filesystem upload, or font generation are
needed. After upload, the program survives unplugging. A normal sketch upload
keeps saved settings when the flash layout is unchanged and erasing is disabled.

**Checkpoint:** AirTraffic reaches its setup screen or radar. Continue with
[4. First boot and Wi-Fi](04-first-boot.md).

## Ready-made firmware (optional alternative)

Skip this section if you followed the Arduino IDE steps. It is for someone who
wants to install a release without compiling or editing the source. A release
can be older than the code on `main`; use the IDE route to build current changes.

On [GitHub Releases](https://github.com/mcconnellcs/AirTraffic/releases), open
**Assets** and download `BoardTest.ino.merged.bin` and
`AirTraffic.ino.merged.bin` from the **same release** into Downloads. These
images are only for the board listed in [What you need](01-what-you-need.md).

On macOS, open **Terminal** from Applications ▸ Utilities and run
`python3 --version`. **esptool 5 requires Python 3.10 or newer.** The Python
3.9 bundled with some Mac developer tools is too old even though that command
works. Install a current Python 3 from
[python.org](https://www.python.org/downloads/macos/) if needed, reopen Terminal,
and confirm the version before running each line below separately:

```sh
cd ~/Downloads
python3 -m venv airtraffic-flash-env
./airtraffic-flash-env/bin/python -m pip install 'esptool==5.4.0'
./airtraffic-flash-env/bin/python -m serial.tools.list_ports
```

Close Serial Monitor. Replace `/dev/cu.usbserial-XXXX` below with the board's
actual port from the last command, then upload the test image:

```sh
./airtraffic-flash-env/bin/python -m esptool --chip esp32s3 --port /dev/cu.usbserial-XXXX --baud 460800 write-flash 0x0 BoardTest.ino.merged.bin
```

Press RST and complete the colour/motion/touch checks above. Then:

```sh
./airtraffic-flash-env/bin/python -m esptool --chip esp32s3 --port /dev/cu.usbserial-XXXX --baud 460800 write-flash 0x0 AirTraffic.ino.merged.bin
```

Use **only the `.merged.bin` image at address `0x0`**. A plain `.ino.bin` is
not interchangeable. A merged image also writes the intervening flash sectors,
including the settings area; expect to set up Wi-Fi again. Press RST if it does
not start automatically. These commands use esptool 5.4.0's
[documented `write-flash` syntax](https://docs.espressif.com/projects/esptool/en/latest/esp32s3/esptool/basic-commands.html).
