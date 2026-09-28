# 2. Install the Arduino IDE

The Arduino IDE turns the project's source code into a program for the board.
Use the exact versions below for your first build; newer versions have not
necessarily been tested together.

## Step 1 — Install Arduino IDE 2 on the Mac

1. Choose **Apple menu ▸ About This Mac**. Note whether it says **Chip: Apple
   M…** (Apple Silicon) or **Processor: Intel**.
2. Download **Arduino IDE 2** for that Mac from
   [Arduino's software page](https://www.arduino.cc/en/software).
3. Open the downloaded `.dmg`, drag **Arduino IDE** into **Applications**,
   then open it from Applications. Follow macOS's normal app-open prompts.
4. Allow any initial downloads to finish. An empty sketch with `setup()` and
   `loop()` is normal. A *sketch* is Arduino's name for a program.

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

## Step 4 — Connect the board and select its port

1. Before connecting, look at **Tools ▸ Port** (it may be absent if there are
   no ports). Then plug the board into the Mac with the data cable.
2. Open **Tools ▸ Port** again. Select the new `/dev/cu.usbserial-…` or
   `/dev/cu.wchusbserial…` entry. The ending varies; do not copy someone else's
   port name. Ignore Bluetooth and debug-console entries.
3. If unsure, unplug the board and see which entry disappears, then reconnect.
   A factory demo on the screen, or the IDE calling the board **Unknown**, is normal.

On Windows the port is usually `COM3`, `COM5`, etc.; on Linux, `/dev/ttyUSB0`.

**No new port?** First try a known data cable and a direct USB connection or
another adapter. This board uses a CH340-family USB-to-serial bridge. If macOS
still cannot see it, use the chip maker's
[WCH Mac driver and instructions](https://github.com/WCHSoftGroup/ch34xser_macos).
Follow any approval/restart steps in that installer. For Windows use
[WCH's CH340 driver](https://www.wch-ic.com/downloads/CH341SER_EXE.html).
See [port troubleshooting](troubleshooting.md#uploading) before proceeding.

## Step 5 — Set the board options

Choose **Tools ▸ Board ▸ esp32 ▸ ESP32S3 Dev Module** first. Then set these
options in **Tools**; the menu may need scrolling:

| Setting | Value |
|---|---|
| Board | **ESP32S3 Dev Module** |
| USB CDC On Boot | **Disabled** |
| CPU Frequency | **240MHz (WiFi)** |
| Flash Mode | **QIO 80MHz** |
| Flash Size | **16MB (128Mb)** |
| Partition Scheme | **16M Flash (3MB APP/9.9MB FATFS)** |
| PSRAM | **OPI PSRAM** |
| Upload Mode | **UART0 / Hardware CDC** |
| USB Mode | **Hardware CDC and JTAG** |
| Arduino Runs On | **Core 1** |
| Events Run On | **Core 1** |
| Upload Speed | **460800** (try **115200** if uploads fail) |
| Erase All Flash Before Sketch Upload | **Disabled** |
| Port | The board's port from Step 4 |

Leave other options at their defaults. **OPI is the PSRAM setting; Flash Mode
stays QIO 80MHz.** USB CDC must stay disabled so Serial Monitor uses the board's
USB-to-serial bridge. A guide for a different ESP32-S3 board may say otherwise.

PSRAM is extra working memory. One 480 × 480 image uses 460,800 bytes, so this
project needs it enabled. These options apply to **both** sketches in the next
step; check them again whenever you switch sketch windows.

**Checkpoint:** ESP32 **3.3.12**, all three exact library versions, the correct
board settings, and a real USB serial port are selected.

Next: [3. Flash it](03-flash-it.md)
