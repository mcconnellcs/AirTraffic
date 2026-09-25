# 2. Install the Arduino IDE

The Arduino IDE is the free program that turns the code into something the board
can run and copies it over the USB cable. About 15 minutes, mostly downloading.

## Step 1 — Get the Arduino IDE

1. Go to <https://www.arduino.cc/en/software> and download **Arduino IDE 2**
   for your computer.
2. Install it and open it. You'll see an empty program with `setup()` and
   `loop()` in it. Don't worry about it.

## Step 2 — Teach it about ESP32 boards

Out of the box the IDE only knows about official Arduino boards. This adds the
ESP32 family:

1. Open **File ▸ Preferences** (on a Mac: **Arduino IDE ▸ Settings**).
2. Find the box called **Additional boards manager URLs** and paste this in:

   ```
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```
3. Click OK.
4. Open **Tools ▸ Board ▸ Boards Manager…**. Search for **esp32**.
5. Install **"esp32" by Espressif Systems**. Pick version **3.3.x** (we tested
   with 3.3.12). This download is big (several hundred MB), so go make a snack.

## Step 3 — Install three libraries

Libraries are code other people wrote that we build on top of. Open
**Tools ▸ Manage Libraries…** and install each of these (search by name):

| Search for      | Install                              | What it does                        |
|-----------------|--------------------------------------|-------------------------------------|
| `LovyanGFX`     | **LovyanGFX** by lovyan03, 1.2.x     | Draws on the screen, reads touches  |
| `ArduinoJson`   | **ArduinoJson** by Benoit Blanchon, 7.x | Reads the flight data we download |
| `WiFiManager`   | **WiFiManager** by tzapu, 2.0.17      | The Wi-Fi setup page on your phone  |

If the IDE asks "Install dependencies?", say **Install all**.

## Step 4 — Plug in the board and find its port

1. Connect the board to your computer with the USB-C cable. The screen may light
   up with whatever program was on it in the factory — that's normal.
2. Open **Tools ▸ Port**. You should see a new entry:
   - Windows: `COM3`, `COM5`, ... (a number that wasn't there before)
   - Mac: `/dev/cu.usbserial-xxxx` or `/dev/cu.wchusbserial-xxxx`
   - Linux: `/dev/ttyUSB0`
3. Select it.

**No new port?** The board uses a **CH340** USB chip. Windows 10/11 and recent
macOS usually have the driver built in; if not, search for "CH340 driver" and
install it from the chip maker (WCH). Then try another USB cable and another
USB socket. On Linux, add yourself to the `dialout` group and log out and in.

## Step 5 — Choose the board settings

These matter. Open the **Tools** menu and set:

| Setting            | Value                                     |
|--------------------|-------------------------------------------|
| Board              | **ESP32S3 Dev Module** (under "esp32")    |
| Flash Size         | **16MB (128Mb)**                          |
| Partition Scheme   | **16M Flash (3MB APP/9.9MB FATFS)**       |
| PSRAM              | **OPI PSRAM**  ← the one people forget    |
| Upload Speed       | 921600 (drop to 460800 if uploads fail)   |

Leave everything else as it is.

> **What's PSRAM?** Extra memory chips on the board. A 480x480 picture needs
> 460,000 bytes and the ESP32 itself only has about 300,000 to spare, so without
> PSRAM turned on the screen can't even hold one picture.

Next: [3. Flash it](03-flash-it.md)
