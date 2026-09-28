# Troubleshooting

Start with **Tools ▸ Serial Monitor**, **115200 baud**, and **New Line**.
Press and release **RST** (sometimes labelled EN) to see messages from startup.
`status` and `help` work in AirTraffic; BoardTest only prints its test report.
Unfamiliar word? See the [glossary](glossary.md).

> **How to debug anything.** Something not working is a normal part of every
> project, not a sign you did it wrong. Four habits solve most problems:
>
> 1. **Read the message.** The first error, or the last line before it
>    stopped, usually says what is wrong.
> 2. **Test the simplest thing.** Does BoardTest work? Does another cable?
> 3. **Change one thing at a time**, and test after each change. Change three
>    things at once and you won't know which one fixed it.
> 4. **Go back to what last worked**, then move forward in small steps.

## Uploading

**No port appears / “No serial port selected”.**

1. Use a known USB **data** cable. A lit screen proves power, not data.
2. Try a direct connection, another USB socket, or a different data-capable adapter.
3. Unplug/replug and compare **Tools ▸ Port**. On Mac choose the new
   `/dev/cu.usbserial-…` or `/dev/cu.wchusbserial…` port, not Bluetooth.
4. If needed, follow the official [WCH Mac driver instructions](https://github.com/WCHSoftGroup/ch34xser_macos)
   or install [WCH's Windows driver](https://www.wch-ic.com/downloads/CH341SER_EXE.html).
   Reopen the IDE after installation and any requested restart.

On Linux, a visible port with **Permission denied** may require serial-group
access. On Debian/Ubuntu, `sudo usermod -a -G dialout "$USER"`, followed by
logging out and back in, usually supplies it. Other distributions may use a
different group. This command is **not for macOS**.

**“Port busy”, “Resource busy”, or “Could not open port”.**
Close other Serial Monitors, Terminal serial programs, and screenshot tools.
Only one program can use the port at a time. Recheck the port after reconnecting.

**Stuck at “Connecting…” / “Failed to connect to ESP32-S3”.**
Close Serial Monitor. Hold **BOOT**, tap and release **RST**, release BOOT,
then click Upload. Try Upload Speed **115200**. After a successful upload,
press RST with BOOT released. Do not hold BOOT during normal startup.

**“Sketch too big” / “text section exceeds available space”.**
Set **Flash Size: 16MB (128Mb)** and **Partition Scheme: 16M Flash (3MB
APP/9.9MB FATFS)** in the window for the sketch you are uploading.

**“No such file or directory”, or errors mentioning LovyanGFX/ArduinoJson.**
Check the [exact library and core versions](02-install-arduino.md), not just
the library names. Keep each `.ino` with all the `.h` and `.cpp` files in its
original folder. Open the extracted ZIP's sketch, not a copy of the `.ino`.
If the IDE says “Multiple libraries were found”, check the **Used** path;
remove an obsolete duplicate only after identifying it. Read the **first**
compiler error; the final `exit status 1` is only a summary.

**It uploaded, but Serial Monitor is blank / commands do nothing.**
Select the board's port, set **115200 baud**, and press RST. **USB CDC On Boot
must be Disabled** for this board's serial bridge; changing it requires another
upload. Set the line-ending selector to **New Line**, type `help`, and press
Return. AirTraffic commands are lowercase. BoardTest does not accept commands.

## The screen

**Blank screen or a “STOP” / “FAIL” message.**
Check **OPI PSRAM** and upload again. Both sketches need PSRAM even before they
can draw their first complete screen. The firmware stops if required startup
resources fail; read Serial Monitor rather than waiting for the screen.
Run BoardTest before debugging AirTraffic. Its report should show 16 MB flash
and nonzero PSRAM. The backlight is on/off through GPIO 38 (one of the chip's
pins), not PWM dimming (flickering a light faster than the eye can see to make
it look dimmer), so there is no brightness setting.

**It restarts repeatedly / “Guru Meditation Error” / brownout message.**
Recheck all [board settings](02-install-arduino.md#step-5--set-the-board-options).
Try a short data cable and a direct computer USB port; insufficient power can
also cause resets. If it persists, save the startup log and backtrace for an issue.

**The picture is sideways or upside down.**
BoardTest uses native orientation. AirTraffic uses `SCREEN_ROTATION = 3` by
default. Change that constant in `firmware/AirTraffic/board_config.h` to
0, 1, 2, or 3, then upload AirTraffic again. Its touch coordinates turn with it.

**Rows judder or jump sideways.**
The display shares PSRAM bandwidth with other work. The tested pixel clock
(`cfg.freq_write` in `board_config.h`) is 12 MHz. First check the power/cable
and exact versions. For a persistent hardware-specific issue, try 11 MHz
(`11000000`) and retest with BoardTest before changing the main app.

**Wrong colours / shifted picture / no touch.**
Confirm the exact board revision first. In BoardTest, a real touch should
print `Touch at x,y` and move the orange dot. If not, the GT911 may use address
`0x14` instead of `0x5D`. Try that change in
`firmware/BoardTest/board_config.h`, upload, and test. If it works, make the
same change in `firmware/AirTraffic/board_config.h` before uploading AirTraffic.
Each sketch has its own copy of the hardware configuration. For other pin
changes, get the seller's configuration for the exact board; do not guess pins.

## Wi-Fi

**I cannot see AirTraffic-Setup.**
On a fresh board it appears shortly after startup. If a saved network cannot
be reached, allow roughly **30 seconds plus scanning time** for the hotspot.
If the board is already connected, open Settings by pressing and holding on
the radar/list/card, then choose **Wi-Fi & location**. On the phone, look at
all networks, including ones without internet.

**Setup worked, then the board went back to setup.**
Opening a serial program can restart this board through the USB bridge. Finish
saving the phone form before opening/closing Serial Monitor or screenshot tools.
If setup was interrupted, rejoin AirTraffic-Setup and save again. If it still
cannot connect, check the Wi-Fi password and 2.4 GHz network, then use the log
to distinguish a restart from a connection failure.

**The page does not pop up / the phone leaves the hotspot.**
Stay connected despite **No Internet** and enter **http://192.168.4.1** in the
address bar. Temporarily disable a phone VPN or automatic cellular fallback
if it routes away from the board. Rejoin normal Wi-Fi when finished.

**My network is missing / the password will not connect.**
Check that the router has **2.4 GHz** enabled and that the password is exact
(including capitals). A shared 2.4/5 GHz network name is normally fine. Move
closer to the router. A normal WPA2-Personal home or guest network is a useful
comparison if a school/work network requires extra authentication. Rejoin
AirTraffic-Setup and try again; a wrong password does not require reflashing.

**It was working but has lost Wi-Fi.**
Check the router, then restart the board. If its saved network still fails,
wait for AirTraffic-Setup and enter the current credentials. Touch controls
are only read on the radar/list/card, so do not try long-pressing the boot
animation. If you are on the radar, Settings can reopen setup directly.

**Slow downloads or weak signal near an access point.**
Use `status` for signal strength and `scan` for visible networks. About −30 dBm
is strong; −70 dBm is weak. Scanning briefly pauses the UI. At startup,
AirTraffic looks for the strongest access point with the saved name; with a
weak connection it checks again at most every ten minutes. Restarting can
help after moving the board.

**I need to clear all settings.**
As a last resort, set **Tools ▸ Erase All Flash Before Sketch Upload ▸ Enabled**
for one upload of AirTraffic. This removes Wi-Fi and all saved settings. Then
**set it back to Disabled** so future uploads do not keep erasing them.
Use the same partition scheme as the setup guide.

## Data

**“No aircraft” but I can see a plane.**
A live feed can legitimately be empty. Confirm the radar centre and increase
RANGE. Volunteer receiver coverage is incomplete; an aircraft may lack usable
ADS-B position reports or be outside reception. Send `demo` to check the display
with simulated aircraft. Missing planes are not proof of a bad build.

**Wrong city / “can't find location”.**
IP-based location can be far away or its service can be unavailable. Use
Settings ▸ **Wi-Fi & location** and enter **both** decimal coordinates.
Check latitude/longitude order and signs. Invalid or incomplete entries revert
to automatic location. Manual coordinates let flight lookup proceed even when
automatic geolocation fails.

**“RETRYING” or feed errors.**
The board retries after a pause of about ten seconds; slow downloads can make
it longer. Read `[net]` messages. Every reply from a website starts with a
three-digit *status code*:

| Code | Meaning |
|---|---|
| 200 | OK |
| 403 | Forbidden: the site refused the request |
| 404 | Not found: for example, no route is known for that flight |
| 429 | Too many requests: you are being *rate limited* |
| 500–599 | The website itself has a problem |

A **DNS** failure means the board could not look up the site's address, which
points to your network rather than the site. Both feeds are external services and can be unavailable.
Do not shorten the refresh interval to work around this. Existing positions
may remain visible and are not fresh while errors continue.

**Missing route, photo, or airline logo.**
These are optional lookups, not a build requirement. Many aircraft have no
matching data. A failed detail lookup can be cached for that session; restarting
can retry it. Demo flights have no photos; logos require internet even in demo.

**Missing or incorrect clock.**
Time is fetched via NTP (UDP port 123). A blocked network can prevent it from
appearing. The time-zone offset comes from IP geolocation, not manual radar
coordinates, and falls back to UTC if that lookup fails with manual coordinates.
Restart after a daylight-saving change to refresh the offset. The 12/24-hour
setting changes formatting only.

## Still stuck?

Ask for help at [GitHub Issues](https://github.com/mcconnellcs/AirTraffic/issues).
Include the board model/revision, macOS/IDE versions, exact core/library
versions, Tools settings, what happened, and the first compiler error or
startup log. Include whether BoardTest's colour/motion/touch checks passed.

Before posting logs, remove passwords if present, Wi-Fi network names,
access-point addresses, IP addresses, and home coordinates you do not want
public. `status`, `scan`, and download URLs can reveal location/network details.

[Back to the build guide](03-flash-it.md) · [Back to README](../README.md)
