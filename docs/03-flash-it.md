# 3. Flash it

*Compile* means turning code into a program. *Upload* (or *flash*) means
copying that program to the board. Upload replaces the factory demo.
A program that lives inside a device like this is called *firmware*.
Unfamiliar word? See the [glossary](glossary.md).

> **Why "flash"?** The board's storage is *flash memory*, the same kind used
> in USB sticks and phones, so writing a program into it became known as
> "flashing". It keeps the program with the power off, which is why you only
> upload once and the radar then starts by itself every time it is plugged in.

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

> **Why so many files?** A program this size is split into pieces that each
> do one job: one file knows about Wi-Fi, another draws the radar, another does
> the map maths. That makes each piece small enough to understand and lets you
> change one without breaking the others. The `.ino` file is only the front
> door. The Arduino IDE builds every file in the sketch's folder together.
> [How it works](05-how-it-works.md#where-things-live) lists what each one does.

## Step 2 — Upload BoardTest

> **Why test the board first?** BoardTest is about 100 lines and uses no
> Wi-Fi. If it fails, the problem must be the board, the cable or the Tools
> settings, because there is almost nothing else it could be. If you went
> straight to AirTraffic and saw a blank screen, there would be dozens of
> possible causes. Testing the simplest thing first, and changing one thing at
> a time, is the most useful debugging habit there is.

1. In Arduino IDE choose **File ▸ Open…**, navigate into the extracted folder,
   and open **firmware ▸ BoardTest ▸ BoardTest.ino**.
2. Recheck the [Tools settings](02-install-arduino.md#step-5--set-the-board-options),
   including the port and **OPI PSRAM**.
3. Click the **✓ Verify** button at the top left. Wait for compilation to
   finish. If it fails, fix the **first error** using
   [Troubleshooting](troubleshooting.md); do not proceed to AirTraffic yet.
   (*Verify* only compiles. It checks the code and your settings without
   touching the board, so it is always safe to press. One mistake often causes
   a whole list of errors; fixing the first usually clears the rest.)
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

> **What each check proves.** The screen receives every pixel's colour over
> 16 separate wires: 5 for red, 6 for green, 5 for blue. A solid red bar
> proves the red wires work, and so on; white uses all 16 at once. A colour
> that looks wrong points to a wiring or settings problem. **FPS** is *frames
> per second*: how many complete pictures are drawn each second.
>
> **Why 115200?** *Baud* is the speed of the serial link in bits per second.
> The program on the board sends at 115200, so Serial Monitor must listen at
> 115200. If the two disagree you get a stream of nonsense characters, a bit
> like playing a record at the wrong speed. Nothing is broken; change the
> number and press RST.
>
> **Why 16777216?** Computers count in powers of two. 16 MB is
> 16 × 1024 × 1024 bytes.

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

<details>
<summary><b>Learn more:</b> what BOOT and RST actually do</summary>

When the ESP32 starts, the first thing to run is a tiny built-in program
called the *bootloader*. It makes one decision: start the program stored in
flash, or wait to receive a new one over the serial connection (*download
mode*). It decides by checking one pin, **IO0**, at the moment of reset.

- **RST** (reset, also labelled **EN**) restarts the chip, like switching it
  off and on.
- **BOOT** is wired to IO0. Holding it down while the chip resets tells the
  bootloader "wait for a new program".

Normally you never touch them. The CH340 chip has two spare control lines,
and the uploader uses them to press both buttons electronically. That is what
`Hard resetting via RTS pin…` means at the end of an upload. The manual
sequence is only needed when that automatic trick fails.

</details>

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
5. **Which way is up?** AirTraffic is drawn a quarter-turn round from
   BoardTest, so that it is upright in the stand this project uses. Turn the
   board until the AIRTRAFFIC title is at the top. If BoardTest looked
   upright and AirTraffic looks sideways, nothing is wrong. To draw it a
   different way round, see `SCREEN_ROTATION` in
   [Troubleshooting](troubleshooting.md#the-screen).

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

> **What these commands mean.** `python3 -m venv` makes a *virtual
> environment*: a private folder for Python tools, so installing esptool
> cannot disturb anything else on the Mac. **esptool** is Espressif's
> uploader, the same one the Arduino IDE runs behind the scenes. `0x0` is an
> address in the flash memory, written in *hexadecimal* (base 16, marked by
> `0x`); it means "start writing at the very beginning". A *merged* image is
> the bootloader, the partition table and the program joined into one file,
> which is why it must start at zero.

Use **only the `.merged.bin` image at address `0x0`**. A plain `.ino.bin` is
not interchangeable. A merged image also writes the intervening flash sectors,
including the settings area; expect to set up Wi-Fi again. Press RST if it does
not start automatically. These commands use esptool 5.4.0's
[documented `write-flash` syntax](https://docs.espressif.com/projects/esptool/en/latest/esp32s3/esptool/basic-commands.html).
