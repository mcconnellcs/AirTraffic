# 1. What you need

You will plug a finished touchscreen board into a computer and upload a program.
No soldering, loose wires, antenna, server, or paid account is needed.
Allow an afternoon for your first build; downloads and troubleshooting can take time.

New word? Every abbreviation in these guides is explained in the
[glossary](glossary.md). Notes marked **Why?** explain the reason behind a
step. **Learn more** sections are optional reading: click to open them.

## Check the board before starting

Use the **ESP32-4848S040C_I** capacitive-touch model used by this project.
The printed board name may shorten this to `ESP32-4848S040`. Check the seller's
specifications as well:

- **4.0-inch, 480 × 480** display, with an **ST7701S** display controller.
- **ESP32-S3**, **16 MB flash**, **8 MB OPI PSRAM**.
- **GT911 capacitive touch**.

Guition lists the C_I model and its memory in its
[manufacturer model table](https://www.guition.com/model-selection).
Listings under AITRIP, Guition, or Sunton names can be similar, but a brand name
or screen size alone does **not** guarantee the same wiring. Other ESP32 display
boards are not supported by these instructions. If you already have the same
board as the project's working build, use that one.

> **Why does the exact model matter?** The program has to know which of the
> chip's pins is wired to which part of the screen. That wiring is decided by
> whoever designed the circuit board, and two boards that look identical can be
> wired differently. With the wrong wiring the screen stays blank or shows
> scrambled colours.

Prices vary by seller, shipping, and whether a case is included. The board,
USB cable, and any adapter are the only required hardware purchases.

<details>
<summary><b>Learn more:</b> what the board's name and specifications mean</summary>

**The name is a description.** `ESP32-4848S040C_I` breaks down as: `ESP32`
(the chip family), `4848` (480 × 480 pixels), `S` (the ESP32-**S**3 chip),
`040` (4.0 inches), `C` (**c**apacitive touch).

**A microcontroller, not a computer.** The ESP32-S3 is a *microcontroller*: a
processor, memory and Wi-Fi radio on a single chip the size of a fingernail.
Unlike a laptop or a Raspberry Pi it has no operating system. It runs exactly
one program, which starts the instant power arrives. That is why the radar is
ready a few seconds after you plug it in, and why nothing can "crash the
desktop".

**Two kinds of memory.** The specifications list two numbers:

| | Flash (16 MB) | PSRAM (8 MB) |
|---|---|---|
| Like a computer's… | SSD / hard drive | RAM |
| Keeps its contents without power? | Yes | No |
| Holds | The program, your Wi-Fi details and settings | The picture being drawn, downloaded flight data |

The chip also has about half a megabyte of fast built-in RAM. One full-screen
picture needs 460,800 bytes (480 × 480 pixels × 2 bytes each), nearly all of
it, which is why this project cannot work without the extra PSRAM chip.

**The other two chips.** The **ST7701S** drives the LCD glass and the
**GT911** reads the touch panel. You never talk to them directly; the
LovyanGFX library does.

</details>

## A Mac laptop and data cable

- A Mac that can run [Arduino IDE 2](https://support.arduino.cc/hc/en-us/articles/360019833020-Download-and-install-Arduino-IDE).
- Permission to install applications (and a USB driver if needed). A managed
  school laptop may need an administrator's help.
- Internet access for the IDE, board package, libraries, and live flight data.
- A **USB-C cable that carries data**, with a connector that fits your Mac.
  A USB-C hub or USB-A adapter must also support data. A charge-only cable can
  light the screen but will never let you upload code.
- Several GB of free disk space for the ESP32 development tools.

> **Why do some cables not work?** A USB cable needs wires for power *and*
> separate wires for data. Cheap cables sold for charging often leave the data
> wires out to save money. They look identical from the outside. The cable
> that came with a phone or an external drive is usually a data cable.

The main instructions use macOS. Windows and Linux differences are included
where needed. No Terminal commands are required for the main build.

## Wi-Fi and a phone

Use a **2.4 GHz home Wi-Fi network** with a name and password, plus a phone
or another Wi-Fi device to open the setup page. The ESP32-S3 cannot join a
5 GHz-only network. A router using one name for both bands can work normally;
you do not need to rename your network just for this project.

> **Why 2.4 GHz?** The radio built into the ESP32-S3 only works on the 2.4 GHz
> band. It was designed to be small, cheap and low-power, and a 2.4 GHz-only
> radio is all three. The radar only downloads a few kilobytes every ten
> seconds, so it has no use for the extra speed of 5 GHz.

<details>
<summary><b>Learn more:</b> 2.4 GHz and 5 GHz Wi-Fi compared</summary>

Wi-Fi is radio. "2.4 GHz" and "5 GHz" are the two ranges of frequency (*bands*)
that most home Wi-Fi uses. Most routers broadcast on both at once, and phones
and laptops pick whichever is better at that moment.

| | 2.4 GHz | 5 GHz |
|---|---|---|
| Range | Longer | Shorter |
| Through walls and floors | Better | Worse |
| Top speed | Slower | Faster |
| Interference | More: shared with Bluetooth, microwave ovens, baby monitors, and every neighbour's Wi-Fi | Less: many more channels to spread across |
| Typical users | Smart plugs, sensors, thermostats, this radar | Phones, laptops, TVs, game consoles |

**Why the difference?** Lower-frequency radio waves bend round obstacles and
pass through walls more easily, so 2.4 GHz reaches further. Higher frequencies
can carry more data each second but are absorbed more quickly. It is the same
reason you hear the bass from a neighbour's music but not the vocals.

**Channels.** Each band is divided into channels, like lanes on a road. In
the USA the 2.4 GHz band has 11 of them, but they overlap, so only channels
1, 6 and 11 can be used side by side without interfering. In a block of flats
that gets crowded. The 5 GHz band has over twenty channels that don't overlap.
When you type `scan` in the Serial Monitor later, you'll see which channel
each nearby network uses.

**One name, two bands.** If your network has a single name for both bands,
the board simply sees and joins the 2.4 GHz part. Your phone and laptop stay on
the same network and can still use 5 GHz.

**Newer Wi-Fi.** Wi-Fi 6E and Wi-Fi 7 add a third band at 6 GHz. The same
trade-off continues: faster still, shorter range still.

</details>

School/work networks requiring a username, certificate, or browser sign-in
are outside this guide. The setup page supports a normal home network password.
Have that password ready before starting.

> **Why not school Wi-Fi?** Home networks use one shared password
> (*WPA2-Personal*). Schools and offices usually give each person their own
> login (*WPA2-Enterprise*), or make you sign in through a web page. The
> board's setup page only handles the shared-password kind.

## Optional

- A nonconductive stand or case that fits this exact board and leaves the USB
  socket accessible. Keep the exposed circuit board off metal.
- A reliable **5 V USB power supply rated for at least 1 A**, to run it without
  the computer after programming. Power this build through USB; leave the
  board's relay terminals and other connectors unused.

> **Why off metal, and why 1 A?** The back of the board has bare electrical
> contacts. Resting it on something metal can connect two of them together (a
> *short circuit*) and damage it. As for power: volts (V) are the "pressure"
> and must match, which every USB supply does at 5 V. Amps (A) are how much
> current the supply *can* deliver; the board takes only what it needs, so a
> bigger number is fine. The screen's backlight and the Wi-Fi radio together
> can briefly draw more than a weak supply can give, and then the board
> restarts by itself.

**Ready to continue:** correct board, data cable, Mac, Wi-Fi password, and a
phone are available.

Next: [2. Install the Arduino IDE](02-install-arduino.md)
