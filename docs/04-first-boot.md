# 4. First boot and Wi-Fi

On a fresh board, AirTraffic creates a setup hotspot so you can enter Wi-Fi
settings from your phone. You never put the Wi-Fi password in the source code.
If the board remembers a working network, it can skip setup.

![Setup screen](images/setup.png)

## Connect it to your Wi-Fi

1. Keep the board powered. On your phone, join **AirTraffic-Setup** in Wi-Fi
   settings, or scan the on-screen QR code to join it. It has no password.
2. If the phone says **No Internet**, choose to **stay connected**. That is
   expected: this hotspot is only a local setup page.
3. If a page does not open automatically, type **http://192.168.4.1** into
   the browser's address bar. Use `http`, not `https`, and do not search for it.
4. Tap **Configure WiFi**, choose your **2.4 GHz** home network, enter its
   password, and tap **Save**. Optional location/units fields are explained below.
5. Allow up to a minute for connection, location lookup, and the first flight
   download. The radar should appear. If your phone stays on AirTraffic-Setup
   after setup, manually switch it back to your normal Wi-Fi.

Setup sends the password from your phone to the board over a local, open Wi-Fi
hotspot and HTTP page. Do this at home. The board stores the credentials for
reconnecting; the application does not send them to flight-data services or
put them in your repository.

## Location and units

- **Latitude / Longitude:** leave **both** blank for an approximate location
  based on your public internet address. This may be many miles away, especially
  with a VPN or mobile internet. Check the place shown at the top of the radar.
- For an exact radar centre, enter **both** coordinates as decimal numbers:
  latitude first (−90 to 90), longitude second (−180 to 180). For example,
  `32.7763` and `-79.9311`. Use a decimal point and a minus sign for south/west;
  do not include degree symbols, compass letters, or a comma in either box.
  In Google Maps on a computer, right-click a location to see its coordinates.
- A missing or invalid coordinate makes the current firmware use **automatic
  location**. There is no form error message, so confirm the centre afterward.
- **Units:** enter `aviation` (feet, knots, nautical miles) or `metric`
  (metres, km/h, kilometres). Settings on the display can also change this.
  The RANGE button's underlying choices remain 10, 25, 50, and 100 **NM**.

To reopen setup later: press and hold on the radar/list/card, tap
**Wi-Fi & location**, then join AirTraffic-Setup again. Selecting your network
and tapping Save applies the new fields. If you choose Exit without saving,
restart the board if needed to return to its saved connection.

## First success check

Open **Tools ▸ Serial Monitor** in Arduino IDE. Set **115200 baud** and choose
**New Line** in the line-ending selector. Opening Serial Monitor can restart
the board; if the start-up animation plays, **wait until the radar is back**
(about 20 seconds) before going on. Then type `status` into the input box
and press Return. Commands need a line ending; **No line ending** will not work.

The reply is three lines starting with `[status]`. Look for **Wi-Fi
connected**, **feed: live**, and source **adsb.lol** or **adsb.fi**. If it
says `waiting for Wi-Fi`, `locating` or `loading`, the board is still starting:
wait ten seconds and send `status` again. A live feed with zero nearby aircraft is still a successful build.
The green/live indicator confirms a successful download, not complete coverage.

Unplug USB for a few seconds and reconnect it. The board should reconnect
using the saved network. This is the final check that setup was saved.

## Try the demo, even before setting up Wi-Fi

With AirTraffic running, send `demo` in Serial Monitor (115200 baud,
**New Line**). Wait a few seconds for the radar to appear with **Demo mode** shown.
Tap RANGE until it is **50 NM** to see the full initial set of 14 simulated
planes. At smaller ranges, some are deliberately outside the view.

Demo includes simulated routes and aircraft facts. Photos are absent and
logos still need internet. Send `demo` again to return to live data, or press
RST to restart; demo mode is not saved. With no working Wi-Fi, live data must
wait for setup. Demo mode is a useful check when there are no real planes nearby.

## Using the radar

![Radar](images/radar.png)

| Do this | Result |
|---|---|
| Tap a plane | Open its flight card |
| Swipe **down** on the card | Close the card |
| Swipe **left / right** on the card | Next / previous plane, ordered by distance |
| Swipe **left** on the radar | Open the nearby-flight list |
| Swipe **right** on the list | Return to radar |
| Swipe **up / down** on the list | Scroll the list |
| Tap a list row | Open that plane's card |
| Tap **RANGE** on the radar | Cycle 10 → 25 → 50 → 100 nautical miles |
| **Press and hold** on radar/list/card | Settings: units, 12/24-hour display, Wi-Fi/location |

After **one minute without touching the radar**, desk mode cycles through up
to the five nearest planes, one card every **12 seconds**. Touch once to leave
desk mode; that first touch wakes the display instead of selecting a control.
It does not start from the list or while Settings is open.

## What the data means

| Colour | Reported altitude |
|---|---|
| Grey | On the ground |
| Orange | Below 1,000 ft |
| Yellow | 1,000 to below 5,000 ft |
| Green | 5,000 to below 15,000 ft |
| Cyan | 15,000 to below 30,000 ft |
| Purple | 30,000 ft or higher |

Altitude colours use feet even in metric mode. A missing altitude can also
appear orange; check the card rather than assuming it is low. Red pulses mark
reported squawk codes 7500, 7600, or 7700. The demo intentionally includes one.

This is a hobby display, not an air-traffic-control or navigation instrument.
Community coverage varies; some aircraft, routes, logos, and photos will be
missing. Up to **60 nearest reported aircraft** are retained per download.
Positions between downloads and route progress are estimates, and downloaded
reports may already be delayed. Missing details do not mean your build failed.

The clock gets its time from NTP and its time-zone offset from the public-IP
lookup, even when radar coordinates are manual. It does not follow the time
zone of coordinates you enter, and the offset does not automatically update
at a daylight-saving change. Restart afterward to refresh it. If the location
service is unavailable with manual coordinates, the clock uses UTC.

**Something wrong?** Start with [Troubleshooting](troubleshooting.md).

Next: [5. How it works](05-how-it-works.md)
