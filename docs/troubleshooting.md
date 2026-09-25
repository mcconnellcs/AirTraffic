# Troubleshooting

Start with **Tools ▸ Serial Monitor** at **115200 baud**. Nearly every problem
below prints a clue there. Press the little **RST** button on the back of the
board to restart it while you watch.

## Uploading

**The IDE can't find a port / "No serial port selected".**
The board's CH340 USB chip needs a driver on some computers. Install the WCH
CH340 driver, then try a different USB cable (many only carry power) and a
different USB socket. On Linux: `sudo usermod -a -G dialout $USER`, then log
out and in.

**"A fatal error occurred: Failed to connect to ESP32-S3".**
Hold the **BOOT** button on the back, tap **RST**, release BOOT, then click
Upload again. Drop **Tools ▸ Upload Speed** to 460800 or 115200 if it still fails.

**"Sketch too big" or "text section exceeds available space".**
Tools ▸ Partition Scheme must be **16M Flash (3MB APP/9.9MB FATFS)** and
Flash Size **16MB**.

**Errors mentioning `lgfx` or `LovyanGFX`.**
The LovyanGFX library isn't installed, or it's a very old version. Install
1.2.x from the Library Manager.

## The screen

**Nothing on the screen, board seems dead.**
The backlight is switched by GPIO 38 (see `backlight.h`); the firmware drives
it as a plain on/off pin because PWM dimming did not light this board at all.
Check the Serial Monitor. If it prints `=== AirTraffic ===`, the board runs
and the problem is the display. Flash `firmware/BoardTest` — it shows colour
bars with no Wi-Fi or memory tricks involved.

**"start-up problem: check PSRAM is set to 'OPI PSRAM'" in the Serial Monitor.**
Exactly that: **Tools ▸ PSRAM ▸ OPI PSRAM**, then Upload again.

**The board restarts over and over (the start-up animation keeps repeating, or
`Guru Meditation Error` in the Serial Monitor).**
Almost always the PSRAM setting above. If PSRAM is right, note the line after
`Backtrace:` and open an issue on GitHub with it.

**The picture is sideways or upside down for how the board is mounted.**
Change `SCREEN_ROTATION` in `firmware/AirTraffic/board_config.h` (0, 1, 2 or 3
= quarter turns) and upload again. Touch follows automatically.

**Colours are wrong (red shows as blue) or the picture is shifted.**
Your board is a different revision. All the wiring lives in
`firmware/AirTraffic/board_config.h`; compare it with a LovyanGFX or Arduino
config that's known to work for your exact model.

**Touch doesn't respond.**
In BoardTest, touches print `Touch at x,y` in the Serial Monitor. If nothing
prints, try the other I2C address in `board_config.h` (`0x14` instead of `0x5D`).

## Wi-Fi

**I can't see the `AirTraffic-Setup` network.**
It only appears while the board has no saved Wi-Fi (or after you choose
**Wi-Fi & location** in Settings). Restart the board and look again within a
minute. Some phones hide networks with no internet; look under "other networks".

**The setup page doesn't pop up.**
Open a browser and type **192.168.4.1**.

**My Wi-Fi isn't in the list / it connects then fails.**
The ESP32 only does **2.4 GHz**. Many routers broadcast 5 GHz and 2.4 GHz under
one name; if the board can't join, log in to the router and give the 2.4 GHz
network its own name. Also: very long passwords with unusual characters
occasionally trip WiFiManager — try a simpler guest network to check.

**Everything is slow, "RETRYING" appears a lot, or `status` shows a weak signal
even though the board is next to the router.**
Type `status` in the Serial Monitor: it prints the access point the board is
talking to and the signal in dBm (−30 is excellent, −70 is poor). Then type
`scan`. Homes with more than one access point broadcast the same network name
from each, and an ESP32 will happily talk to a far one. AirTraffic joins the
strongest one at start-up and re-checks every 10 minutes when the signal is
poor, so a restart usually fixes it; if `scan` only shows weak ones, move the
board or the access point.

**It was working and now says "Connecting to Wi-Fi" forever.**
Router rebooted or password changed. Press and hold the screen ▸ **Wi-Fi &
location** to set it up again. To wipe everything, in the IDE set
**Tools ▸ Erase All Flash Before Sketch Upload ▸ Enabled** for one upload.

## Data

**"No aircraft within 25 NM" but I can see a plane out the window.**
Small planes without ADS-B (older ones, some military) are invisible to
everyone. Tap **RANGE** to widen the circle. Check the Serial Monitor for
`[net]` errors — the free feeds occasionally go down for a few minutes and the
radar keeps retrying by itself (it also switches to a second feed automatically).

**The radar is centred in the wrong place.**
Automatic location comes from your internet provider and can be a town over.
Type your real latitude/longitude on the setup page (**Wi-Fi & location**).

**The clock isn't shown.**
It appears once the time has been fetched from the internet, a few seconds
after connecting. If it never appears, your network may block NTP (port 123).

**"RETRYING" in the corner.**
A download failed; the next one is 10 seconds later. It's only a problem if it
stays that way — then check `[net]` lines in the Serial Monitor.

## Still stuck?

Open an issue at <https://github.com/mcconnellcs/AirTraffic/issues> and paste:

- what you expected and what happened,
- the Serial Monitor output from the restart onward,
- your Tools menu settings.
