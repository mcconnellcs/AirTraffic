# 6. Level-up missions

The radar works. Now make it *yours*. Each mission tells you exactly which file
to open. Change one thing, click Upload, see what happens. That loop is how
everyone learns this.

The Serial Monitor (**Tools ▸ Serial Monitor**, 115200 baud) shows the board's
messages, and typing `demo` gives you planes to look at even without Wi-Fi.

## Level 1 — Change a number

**Mission 1: Your colours.** Open `theme.h`. Change `kAccent` from the
mint green to something else, for example `rgb(255, 80, 200)`. Upload. The
sweep, the selection brackets and the live dot all follow. Now try the six
`kAltitude` colours.

**Mission 2: Faster sweep.** In `app.cpp`, `kSweepPeriodMs = 4000` is one full
turn in milliseconds. Try 2000. Try 8000. Which feels more like a real radar?

**Mission 3: Longer tail.** In `screen_radar.cpp`, `kSweepTailDeg = 50.0f`
is how far the glow trails behind the sweep line.

**Mission 4: More name tags.** `kMaxLabels = 9` in `screen_radar.cpp` limits
how many planes get a callsign written next to them. Set it to 30 and see why
there's a limit.

**Mission 5: Turn it round.** `SCREEN_ROTATION` in `board_config.h` turns the
whole picture (and touch) in quarter turns: 0, 1, 2 or 3. Handy for a stand
that holds the board sideways.

**Mission 5b: A 5-mile range.** `kRangeChoicesNm` in `app_settings.h` is the
list the RANGE button cycles through. Add `5` at the front.

## Level 2 — Change a behaviour

**Mission 6: Desk mode timing.** `kAmbientAfterMs` in `app_settings.h` is how
long the radar waits before it starts showing flight cards on its own.
`kAmbientCycleMs` in `app.cpp` is how long each card stays. Make it show only
the single nearest plane (hint: `kAmbientNearest`).

**Mission 7: Metric by default.** In `app_settings.cpp`, `load()` uses
`prefs.getBool("metric", false)`. What does changing that `false` do, and why
does it only matter the first time?

**Mission 8: A watchlist.** In `screen_radar.cpp`, `drawBlip()` already draws
emergency planes in red. Add your own rule: if the callsign starts with `"N"`
(a private plane in the USA) or equals a flight your family is on, give it the
glow treatment. `strncmp(b.track->latest.callsign, "N", 1) == 0` is a start.

**Mission 9: Bigger planes far away.** In `drawBlip()`, the plane is drawn with
`planeIcon(..., b.selected ? 26 : 20, ...)`. Make the size depend on altitude
so high cruisers look small and low planes look big (hint: `b.track->latest.altFt`).

## Level 3 — Add something new

**Mission 10: One more stat on the card.** In `screen_card.cpp`, `drawStats()`
builds a list of six `Stat` boxes. Add a seventh: the ICAO type code
(`f.type`), or the aircraft `category` ("A3" = large airliner, "A7" =
helicopter). You'll need to move things around to make it fit — that's design.

**Mission 11: A "closest plane" screen.** Make a new file `screen_nearest.cpp`
with a `drawNearest()` that shows just one plane in huge text
(`theme::Font::Huge` is already there, up to 64 px). Add it as a third page:
look at how `showPage()` in `app.cpp` slides between the radar and the list.

**Mission 12: Your own font.** Drop a `.ttf` file into `tools/fonts/`, add a
line to the `FONTS` table in `make_fonts.py`, run
`python3 tools/fonts/make_fonts.py`, and use the new name in `theme.h`. Fonts
must be free to redistribute (the ones included are under the SIL Open Font
License).

**Mission 13: A case.** Search Printables or Thingiverse for
"ESP32-4848S040 case". If you have a printer, print one; if not, a phone stand
works surprisingly well.

## Level 4 — Ideas that need real thought

- **Notify your phone** when a plane passes overhead. Look up `ntfy.sh` — one
  HTTP request sends a push notification, and `net_client.cpp` already knows
  how to make requests.
- **Show the airline logo** on the card. Where would the pictures come from?
  How big can they be? (The photo box is 150×100 px.)
- **A map behind the radar**. OpenStreetMap serves map tiles as PNG images.
  What would you need to draw one under the planes?
- **Sound.** The board has no speaker, but it has spare pins…

## How to not get stuck

- Change **one thing** at a time, then Upload.
- The Serial Monitor tells you what's happening. `[feed]`, `[app]`, `[net]`
  lines come from those files.
- If it won't compile, read the **first** red line only. The rest is usually
  fallout from that one.
- `git diff` (or GitHub Desktop) shows exactly what you changed since it last
  worked. `git checkout -- filename` puts a file back.
- The tests in `test/` are the safety net for the maths. Run `make -C test`
  after changing `geo.cpp`, `format.cpp` or `sky_model.cpp`.

If you build something good, open a pull request. That's how this project grows.
