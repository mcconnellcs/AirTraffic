# 2. Install the Arduino IDE

The Arduino IDE (Integrated Development Environment) turns the project's
source code into a program for the board. It is three tools in one app: an
**editor** to read and change code, a **compiler** to translate it for the
chip, and an **uploader** to copy it to the board.

Use the exact versions below for your first build; newer versions have not
necessarily been tested together. Unfamiliar word? See the [glossary](glossary.md).

> **Why exact versions?** This project is built on top of four pieces of other
> people's software, and all of them keep changing. A newer version might
> rename a function or change how the screen is driven, and then a project that
> worked last month stops building. Writing down the versions that were tested
> together is called *pinning*, and it means your build is the same build that
> is known to work. Professional projects do exactly this.

## Step 1 — Install Arduino IDE 2 on the Mac

1. Choose **Apple menu ▸ About This Mac**. Note whether it says **Chip: Apple
   M…** (Apple Silicon) or **Processor: Intel**.
2. Download **Arduino IDE 2** for that Mac from
   [Arduino's software page](https://www.arduino.cc/en/software).
3. Open the downloaded `.dmg`, drag **Arduino IDE** into **Applications**,
   then open it from Applications. Follow macOS's normal app-open prompts.
4. Allow any initial downloads to finish. An empty sketch with `setup()` and
   `loop()` is normal. A *sketch* is Arduino's name for a program.

> **Why does the chip matter?** Apple Silicon and Intel Macs use different
> kinds of processor, and a program built for one does not run directly on the
> other. The download page offers a version for each.

Every Arduino program has the same two parts. `setup()` runs once when the
board powers on; `loop()` then runs over and over for as long as it has power.
You will see both in `AirTraffic.ino`.

For supported operating systems, Windows/Linux installation, or an installer
problem, use [Arduino's installation guide](https://support.arduino.cc/hc/en-us/articles/360019833020-Download-and-install-Arduino-IDE).

## Step 2 — Add the ESP32 board package

1. Open **Arduino IDE ▸ Settings…** (may be called **Preferences…** on some
   versions). On Windows/Linux use **File ▸ Preferences**.
2. In **Additional boards manager URLs**, add the following URL. Keep any
   URLs already there; use the list editor to put each on its own line.

   ```text
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```

3. Click **OK**, then open **Tools ▸ Board ▸ Boards Manager…** (or the board
   icon in the left sidebar).
4. Search `esp32`. Select **esp32 by Espressif Systems**, choose **3.3.12** in
   its version selector, and click **Install**. This is different from
   **Arduino ESP32 Boards**.
5. Wait for the installation to finish. The download can take several minutes.
   If you already installed another version, select **3.3.12** and install it.

> **Why is this step needed?** Out of the box the Arduino IDE only knows how
> to build programs for Arduino's own boards. A *board package* (also called a
> *core*) teaches it a new family of chips: it contains the compiler for that
> chip, the uploader, and the list of boards you'll pick from. The URL points
> at a list, published by Espressif (the company that makes the ESP32), of
> where to download each version.
>
> **Why "by Espressif Systems" and not "Arduino ESP32 Boards"?** The second
> one is Arduino's package for their own ESP32-based products. Ours is a
> generic ESP32-S3 board, which only Espressif's package supports.

## Step 3 — Install three libraries

Open **Tools ▸ Manage Libraries…**. Search each name, choose its version in
the selector, and click **Install**:

| Library | Author shown in Library Manager | Version | Purpose |
|---|---|---|---|
| LovyanGFX | lovyan03 | **1.2.30** | Screen and touch |
| ArduinoJson | Benoit Blanchon | **7.4.3** | Read flight data |
| WiFiManager | tzapu | **2.0.17** | Phone setup page |

If asked about dependencies, choose **Install all**. You do not need LVGL,
TFT_eSPI, a separate GT911 library, or Python for the main build.

> **Why use libraries?** A *library* is code someone has already written,
> tested and shared. Drawing text on this screen, reading the JSON data format
> and running a Wi-Fi setup page are each months of work. Thousands of people
> use these three libraries, so their bugs have mostly been found. Knowing
> when to build on someone else's work, and giving them credit, is a big part
> of real programming. They are credited in the [README](../README.md#credits).
>
> **Why check the author?** Library Manager often lists several libraries
> with similar names. The author tells you that you have the right one.

## Step 4 — Connect the board and select its port

1. Before connecting, look at **Tools ▸ Port** (it may be absent if there are
   no ports). Then plug the board into the Mac with the data cable.
2. Open **Tools ▸ Port** again. Select the new `/dev/cu.usbserial-…` or
   `/dev/cu.wchusbserial…` entry. The ending varies; do not copy someone else's
   port name. Ignore Bluetooth and debug-console entries.
3. If unsure, unplug the board and see which entry disappears, then reconnect.
   A factory demo on the screen, or the IDE calling the board **Unknown**, is normal.

On Windows the port is usually `COM3`, `COM5`, etc.; on Linux, `/dev/ttyUSB0`.

> **What is a port?** The board and the Mac talk using *serial* communication:
> data sent one bit after another. The ESP32 chip speaks serial but the Mac
> speaks USB, so the board carries a small translator chip, the **CH340**,
> between them. When you plug in, macOS notices the CH340 and creates a *serial
> port*, an entry such as `/dev/cu.usbserial-110`, which programs open like a
> file to talk to the board. The IDE needs to know which one is your board.

**No new port?** First try a known data cable and a direct USB connection or
another adapter. This board uses a CH340-family USB-to-serial bridge. If macOS
still cannot see it, use the chip maker's
[WCH Mac driver and instructions](https://github.com/WCHSoftGroup/ch34xser_macos).
Follow any approval/restart steps in that installer. For Windows use
[WCH's CH340 driver](https://www.wch-ic.com/downloads/CH341SER_EXE.html).
See [port troubleshooting](troubleshooting.md#uploading) before proceeding.

> **What is a driver?** Software that teaches the operating system how to
> talk to a piece of hardware. Recent versions of macOS already include one
> for the CH340, so most people can skip this.

## Step 5 — Set the board options

Choose **Tools ▸ Board ▸ esp32 ▸ ESP32S3 Dev Module** first. Then set these
options in **Tools**; the menu may need scrolling:

| Setting | Value | Why |
|---|---|---|
| Board | **ESP32S3 Dev Module** | Our board isn't in the list by name. This is the generic entry for any board built around an ESP32-S3; the settings below describe the rest. |
| USB CDC On Boot | **Disabled** | The board talks to the Mac through its CH340 chip, not the ESP32's own USB. Enabled would send all messages to a USB connection that isn't wired up, and Serial Monitor would stay blank. |
| CPU Frequency | **240MHz (WiFi)** | Full speed. Drawing 30 pictures a second while downloading needs all of it. |
| Flash Mode | **QIO 80MHz** | How the chip talks to its flash memory: four data wires (*Quad* I/O) at 80 MHz. |
| Flash Size | **16MB (128Mb)** | How much storage the board has. The IDE assumes 4 MB unless told. (128Mb is the same amount counted in mega*bits*: 16 × 8.) |
| Partition Scheme | **16M Flash (3MB APP/9.9MB FATFS)** | How the storage is divided up. The default only allows 1.2 MB for the program, and AirTraffic is about 1.6 MB. |
| PSRAM | **OPI PSRAM** | Switches on the 8 MB of extra memory and says it is wired with eight data lines (*Octal*). Off by default. |
| Upload Mode | **UART0 / Hardware CDC** | Upload through the serial connection (the CH340). |
| USB Mode | **Hardware CDC and JTAG** | Only matters for boards using the ESP32's own USB. Left at its default. |
| Arduino Runs On | **Core 1** | The chip has two processor cores. The program's main loop draws the screen on core 1… |
| Events Run On | **Core 1** | …which leaves core 0 for Wi-Fi and downloads. See [How it works](05-how-it-works.md). |
| Upload Speed | **460800** (try **115200** if uploads fail) | Bits per second while uploading. Faster is quicker, but a long or poor cable causes errors; slower is more forgiving. |
| Erase All Flash Before Sketch Upload | **Disabled** | Enabled would wipe your saved Wi-Fi details on every upload. |
| Port | The board's port from Step 4 | Which connection to upload through. |

Leave other options at their defaults. **OPI is the PSRAM setting; Flash Mode
stays QIO 80MHz.** USB CDC must stay disabled so Serial Monitor uses the board's
USB-to-serial bridge. A guide for a different ESP32-S3 board may say otherwise.

> **Why so many settings?** The IDE cannot detect what is on the board; it
> only sees a generic ESP32-S3. These options are you *describing the
> hardware* to the compiler. The three that are wrong by default, and cause
> nearly every first-build problem, are **Flash Size**, **Partition Scheme**
> and **PSRAM**.

PSRAM is extra working memory. One 480 × 480 image uses 460,800 bytes, so this
project needs it enabled. These options apply to **both** sketches in the next
step; check them again whenever you switch sketch windows.

<details>
<summary><b>Learn more:</b> how the 16 MB of flash is divided</summary>

Flash memory is split into *partitions*, each reserved for one job, like
drawers in a filing cabinet. The scheme chosen above makes these:

| Partition | Size | What it holds |
|---|---|---|
| `nvs` | 20 KB | **N**on-**V**olatile **S**torage: your Wi-Fi name and password, units, range |
| `otadata` | 8 KB | A note of which program slot to start |
| `app0` | 3 MB | The program (AirTraffic, about 1.6 MB) |
| `app1` | 3 MB | A second program slot, for updating over Wi-Fi. Unused here |
| `ffat` | 9.9 MB | Space for files. Unused here |
| `coredump` | 64 KB | Crash reports, for debugging |

Two things follow from this. Uploading a new program only rewrites `app0`, so
the Wi-Fi details in `nvs` survive, which is why you don't have to set up Wi-Fi
again after every upload. And with the *default* scheme the program slot is
only 1.2 MB, so the IDE stops with **"Sketch too big"**: a confusing message
whose real meaning is "wrong Partition Scheme".

</details>

**Checkpoint:** ESP32 **3.3.12**, all three exact library versions, the correct
board settings, and a real USB serial port are selected.

Next: [3. Flash it](03-flash-it.md)
