# Glossary

Every abbreviation and piece of jargon used in these guides, in plain words.
You don't need to memorise any of it. Look things up as you meet them.

Jump to: [The board](#the-board) · [Memory](#memory) ·
[The Arduino tools](#the-arduino-tools) · [USB and serial](#usb-and-serial) ·
[Wi-Fi and the internet](#wi-fi-and-the-internet) · [Aviation](#aviation) ·
[Screen and graphics](#screen-and-graphics) · [Programming words](#programming-words)

## The board

| Term | What it means |
|---|---|
| **Microcontroller** | A whole small computer on one chip: processor, memory and input/output pins. It runs one program, starting the moment it gets power. No operating system, no desktop. |
| **ESP32** | A family of microcontrollers made by Espressif, popular because they have Wi-Fi built in and cost a few dollars. |
| **ESP32-S3** | The member of that family on this board: two processor cores running at up to 240 MHz, with Wi-Fi and Bluetooth. |
| **ESP32-4848S040** | This board's model name. `4848` = 480 × 480 pixels, `S` = uses an ESP32-**S**3, `040` = 4.0 inch screen. The `C` in `C_I` means **c**apacitive touch. |
| **Core** (processor) | One processor inside the chip. The ESP32-S3 has two, so it can do two things at the same moment. |
| **MHz / GHz** | Megahertz and gigahertz: millions or billions of cycles per second. Used for processor speed (240 MHz) and for radio frequency (2.4 GHz). |
| **GPIO** | General-Purpose Input/Output: a pin on the chip that the program can switch on or off, or read. The backlight is on GPIO 38. |
| **Capacitive touch** | Touch sensing that detects the tiny electrical effect of your finger, like a phone. It needs only a light touch. (*Resistive* screens need pressure.) |
| **GT911** | The chip on this board that reads the touch panel and tells the ESP32 where your finger is. |
| **ST7701S** | The chip that drives the LCD glass. The ESP32 sends it pixels; it puts them on the screen. |
| **I2C** | A two-wire connection for chips to talk to each other slowly. The touch chip uses it. Said "eye-squared-see". |
| **Relay** | An electrically operated switch. This board has connectors for them; AirTraffic does not use them. |
| **Brownout** | The supply voltage dipping too low for the chip to work reliably, usually from a poor cable or weak power supply. The chip restarts to protect itself. |

## Memory

| Term | What it means |
|---|---|
| **Flash** | Storage that keeps its contents with the power off, like a computer's SSD. Your program lives here. This board has 16 MB. |
| **RAM** | Working memory: fast, but wiped when the power goes off. The ESP32-S3 has about 0.5 MB built in. |
| **PSRAM** | Pseudo-Static RAM: an extra memory chip on the board adding 8 MB of working memory. Bigger than the built-in RAM but slower to reach. |
| **MB / KB** | Megabyte and kilobyte. 1 KB is about a thousand bytes, 1 MB about a million. One screen image here is 460 KB. |
| **QIO** | Quad I/O: the flash chip is wired with **four** data lines, so it moves four bits at a time. |
| **OPI** | Octal Peripheral Interface: the PSRAM chip is wired with **eight** data lines. Faster than quad, and the IDE must be told which one the board has. |
| **Partition** | A section of the flash reserved for one job, like a drawer in a filing cabinet: one for the program, one for settings, and so on. |
| **Partition scheme** | The plan for how the flash is divided up. |
| **NVS** | Non-Volatile Storage: the small partition where the board keeps your Wi-Fi details and settings. |
| **FATFS** | A simple file system (the kind used on USB sticks). The partition scheme reserves space for one; AirTraffic doesn't use it. |
| **Bootloader** | A tiny program that runs first at power-on and starts your program. It can also receive a new program over USB. |

## The Arduino tools

| Term | What it means |
|---|---|
| **Arduino** | A family of beginner-friendly tools and a style of programming for microcontrollers. It started with Arduino's own boards and now supports many others. |
| **IDE** | Integrated Development Environment: one app containing an editor, a compiler and an uploader. |
| **Sketch** | Arduino's name for a program. The main file ends in `.ino`. |
| **Compile** (Verify) | Translate code that people can read into instructions the chip can run. |
| **Upload** (flash) | Copy the compiled program into the board's flash memory. |
| **Firmware** | Software that lives permanently inside a device. AirTraffic is the firmware for your radar. |
| **Board package** (also "core") | The add-on that teaches the Arduino IDE how to build programs for a family of chips. Here: **esp32 by Espressif Systems**. A confusing second meaning of "core"; see also *Core (processor)*. |
| **Library** | Code someone else wrote and shared so you don't have to write it again. |
| **Dependency** | Something your project needs in order to build: here, the board package and three libraries. |
| **Version** | A numbered snapshot of software, like `1.2.30`. Different versions can behave differently. |
| **Source code** | The human-readable program files (`.ino`, `.cpp`, `.h`). |
| **`.h` and `.cpp` files** | A header (`.h`) lists *what* functions exist; a source file (`.cpp`) holds *how* they work. |
| **Binary** (`.bin`) | A compiled program, ready to upload. Not readable by people. |
| **CLI** | Command-Line Interface: a program you control by typing commands in Terminal instead of clicking. |
| **Terminal** | The Mac app for typing commands. |
| **Git / GitHub** | Git records every change to a project so you can go back. GitHub is a website that stores Git projects and shares them. |
| **Repository** ("repo") | One project stored in Git. |
| **ZIP** | One file containing a whole folder, squeezed smaller for downloading. |
| **CI** | Continuous Integration: a robot on GitHub that rebuilds and tests the project every time it changes. |
| **FQBN** | Fully Qualified Board Name: the board and all its Tools settings written as one line of text, for the CLI. |
| **Virtual environment** ("venv") | A private folder of Python tools for one project, so they can't interfere with anything else on the computer. |

## USB and serial

| Term | What it means |
|---|---|
| **Serial** | Sending data one bit after another down a wire. The simplest way for a board and a computer to talk. |
| **UART** | Universal Asynchronous Receiver/Transmitter: the part of a chip that does serial communication. |
| **Serial port** | How the computer sees the board's serial connection. On a Mac it looks like `/dev/cu.usbserial-110`. |
| **Serial Monitor** | The Arduino IDE window that shows what the board prints and lets you type commands back. |
| **Baud** | Speed of a serial link in bits per second. `115200 baud` is about 11,500 characters a second. Both ends must use the same number. |
| **CH340** | The chip on this board that translates between USB (what the Mac speaks) and serial (what the ESP32 speaks). |
| **Driver** | Software that lets the computer's operating system talk to a piece of hardware. |
| **USB CDC** | A way for a chip to appear as a serial port using its *own* USB hardware. This board uses the CH340 instead, so the setting stays off. |
| **JTAG** | A debugging connection for professionals. Not used here. |
| **Restart** | Unplug the USB cable, wait two seconds, and plug it back in. The program starts again from the beginning. |
| **Download mode** | A state where the chip waits to receive a new program over USB instead of starting the old one. The uploader switches the board into it automatically. |
| **Line ending** | The invisible "Return" character sent after what you type, so the board knows the command is finished. |

## Wi-Fi and the internet

| Term | What it means |
|---|---|
| **Wi-Fi** | Radio networking. Standardised as IEEE 802.11. |
| **2.4 GHz / 5 GHz** | The two radio bands most home Wi-Fi uses. See [What you need](01-what-you-need.md#wi-fi-and-a-phone). |
| **Router / Access point (AP)** | The box your devices connect to. A large home may have several access points sharing one network name. |
| **SSID** | Service Set Identifier: the network's name, the one you pick from the list. |
| **WPA2 / WPA3** | The security that scrambles home Wi-Fi so only people with the password can read it. |
| **Hotspot** | A device acting as its own small Wi-Fi network. The board does this during setup. |
| **Captive portal** | A web page that opens by itself when you join a network, like hotel Wi-Fi sign-in pages. |
| **QR code** | A square barcode a phone camera can read. The one on the setup screen contains "join this Wi-Fi network". |
| **IP address** | The numeric address of a device on a network, like `192.168.4.1`. |
| **DNS** | Domain Name System: the internet's phone book, turning names like `api.adsb.lol` into IP addresses. |
| **URL** | A web address. |
| **HTTP / HTTPS** | The language browsers and websites speak. The **S** means *secure*: encrypted, and the site's identity is checked. |
| **Certificate** | A site's digital ID card, used by HTTPS to prove the site is who it claims to be. |
| **API** | Application Programming Interface: a website made for programs rather than people. It answers with data instead of pages. |
| **JSON** | JavaScript Object Notation: a simple text format for data, like `{"flight": "AAL1699", "alt_baro": 35000}`. |
| **NTP** | Network Time Protocol: how devices ask the internet what time it is. |
| **UTC** | Coordinated Universal Time: the world's reference clock, with no time zones or daylight saving. |
| **dBm** | A measure of radio signal strength. Always negative for Wi-Fi; closer to zero is stronger. −40 is excellent, −70 is weak. |
| **VPN** | Virtual Private Network: routes your internet traffic through another location, which confuses location lookups. |
| **Geolocation** | Working out where you are. *IP geolocation* guesses from your internet address and is often only accurate to the nearest city. |

## Aviation

| Term | What it means |
|---|---|
| **ADS-B** | Automatic Dependent Surveillance–Broadcast. Aircraft work out their own position by GPS and broadcast it by radio about twice a second for anyone to receive. |
| **Transponder** | The radio in an aircraft that sends its identity and position. |
| **Callsign** | What air traffic control calls a flight, e.g. `AAL1673` = American Airlines flight 1673. |
| **Registration** | The aircraft's "number plate", painted on the tail, e.g. `N576UW`. The first letters show the country (`N` = USA, `G` = UK). |
| **Hex code** | A unique 24-bit address built into each aircraft's transponder, written in hexadecimal, e.g. `ad727d`. |
| **ICAO** | International Civil Aviation Organization, the UN body for aviation. *ICAO codes*: 3 letters for airlines (`AAL`), 4 for airports (`KCLT`), and type codes for aircraft (`B738` = Boeing 737-800). |
| **IATA** | International Air Transport Association, the airlines' trade body. *IATA codes* are the ones on tickets and luggage tags: 2 characters for airlines (`AA`), 3 letters for airports (`CLT`). |
| **Squawk** | A four-digit code the pilot sets on the transponder, given by air traffic control. `7500`, `7600` and `7700` are reserved for emergencies. |
| **Nautical mile (NM)** | 1,852 metres, about 1.15 ordinary miles. Chosen because it is one-sixtieth of a degree of latitude, which makes map maths easy. |
| **Knot (kt)** | One nautical mile per hour: about 1.15 mph or 1.85 km/h. |
| **Feet (ft)** | Altitude is measured in feet almost everywhere in aviation, even in countries that use metres for everything else. |
| **fpm** | Feet per minute: how fast an aircraft is climbing or descending. |
| **Flight level (FL)** | Altitude in hundreds of feet on a standard pressure setting. `FL350` = 35,000 ft. Used at high altitude so every aircraft measures the same way. |
| **Heading / Track** | The direction of travel in degrees: 0 = north, 90 = east, 180 = south, 270 = west. |
| **Dead reckoning** | Estimating where something is now from where it was, how fast it was going and in which direction. |
| **ATC** | Air Traffic Control. |

## Screen and graphics

| Term | What it means |
|---|---|
| **Pixel** | One dot on the screen. This screen has 480 × 480 = 230,400 of them. |
| **LCD** | Liquid Crystal Display. |
| **IPS** | In-Plane Switching: a better kind of LCD whose colours stay true when viewed from the side. |
| **Backlight** | The light behind an LCD. The glass only blocks or passes light; without the backlight you see nothing. |
| **RGB** | Red, Green, Blue: every colour on a screen is a mix of these three. |
| **RGB565** | A way to store a colour in 16 bits: 5 for red, 6 for green, 5 for blue. 65,536 possible colours. |
| **FPS** | Frames Per Second: how many complete pictures are drawn each second. Around 30 looks smooth. |
| **Anti-aliasing** | Softening the jagged edges of text and lines by blending edge pixels with the background. |
| **Sprite** | An off-screen picture held in memory, drawn on and then copied to the screen. |
| **PWM** | Pulse-Width Modulation: switching something on and off very fast to make it seem dimmer. |
| **Pixel clock** | How many pixels per second are sent to the screen. |

## Programming words

| Term | What it means |
|---|---|
| **Function** | A named piece of code that does one job, like `drawRadar()`. |
| **Variable / Constant** | A named value. A variable can change; a constant (such as `kSweepPeriodMs`) is fixed when the program is built. |
| **Hexadecimal** ("hex") | Counting in base 16, with digits 0–9 then A–F. Written with `0x` in front: `0x10` is 16. |
| **Bit / Byte** | A bit is a single 0 or 1. A byte is eight bits. |
| **Task** | A piece of the program that runs alongside the others, each taking turns on a processor core. |
| **Mutex** | A lock that makes sure only one task touches a piece of shared data at a time. |
| **Cache** | A small store of recently fetched things, kept so they don't have to be fetched again. |
| **Watchdog** | A timer that restarts the board if the program gets stuck. |
| **Bug / Debugging** | A mistake in a program, and the detective work of finding it. |
| **Unit test** | A small program that checks one piece of code gives the right answer. |
| **Open source** | Software whose source code is published for anyone to read, use and change. |
| **MIT licence** | The licence of this project: do almost anything you like with the code, as long as you keep the copyright notice. |

[Back to README](../README.md) · [Start the build](01-what-you-need.md)
