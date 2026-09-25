# 4. First boot and Wi-Fi

The first time it starts, AirTraffic doesn't know your Wi-Fi yet. Instead of
making you type a password into the code (which you'd then have to keep secret),
it becomes a little Wi-Fi hotspot and asks you from your phone.

![Setup screen](images/setup.png)

## Connect it to your Wi-Fi

1. On your phone, open the Wi-Fi settings and join the network called
   **AirTraffic-Setup**. There's no password. (Or scan the QR code on the screen.)
2. A setup page pops up by itself. If it doesn't within a few seconds, open a
   browser and go to **192.168.4.1**.
3. Tap **Configure WiFi**, pick your home network from the list, type its
   password and tap **Save**.
4. The board connects, finds where it is, and the radar appears. Your phone
   goes back to its normal Wi-Fi on its own.

Your password is saved on the board only. It is never in the code, never on
GitHub and never sent anywhere.

## The optional boxes on the setup page

- **Latitude / Longitude** — leave blank and the radar works out where it is
  from your internet connection (usually within a few miles, good enough).
  Type your exact coordinates (from Google Maps: right-click ▸ the numbers at
  the top) if you want the centre of the radar to be your actual house.
- **Units** — `aviation` (feet, knots, nautical miles, what pilots use) or
  `metric` (metres, km/h). You can change this later on the screen too.

## Using the radar

![Radar](images/radar.png)

| Do this                        | To get                                              |
|--------------------------------|-----------------------------------------------------|
| Tap a plane                    | Its flight card: where it's going, how high, how fast |
| Swipe **down** on the card     | Close the card                                      |
| Swipe **left/right** on the card | Next / previous plane                              |
| Swipe **left** on the radar    | The list of all nearby flights                      |
| Swipe **right** on the list    | Back to the radar                                   |
| Tap **RANGE**                  | Zoom: 10, 25, 50 or 100 nautical miles              |
| **Press and hold** anywhere    | Settings: brightness, units, 12/24h clock, Wi-Fi    |

Leave it alone for a minute and it turns into a desk display, showing the
nearest planes' cards one after another. Touch it to take control again.

## Colours mean altitude

| Colour  | Altitude              |
|---------|-----------------------|
| grey    | on the ground         |
| orange  | below 1,000 ft        |
| yellow  | below 5,000 ft        |
| green   | below 15,000 ft       |
| cyan    | below 30,000 ft       |
| purple  | cruising above 30,000 ft |

A plane pulsing **red** is squawking an emergency code (7500, 7600 or 7700).
That's rare, and usually a test or a mistake, but it's real data.

## No Wi-Fi handy? Try the demo

Open **Tools ▸ Serial Monitor** (115200 baud), type `demo` and press Enter. The
radar fills with 14 pretend planes so you can play with every screen. Type `demo`
again to go back to live data.

Next: [5. How it works](05-how-it-works.md)
