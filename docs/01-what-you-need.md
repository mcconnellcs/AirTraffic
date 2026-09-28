# 1. What you need

You will plug a finished touchscreen board into a computer and upload a program.
No soldering, loose wires, antenna, server, or paid account is needed.
Allow an afternoon for your first build; downloads and troubleshooting can take time.

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

Prices vary by seller, shipping, and whether a case is included. The board,
USB cable, and any adapter are the only required hardware purchases.

## A Mac laptop and data cable

- A Mac that can run [Arduino IDE 2](https://support.arduino.cc/hc/en-us/articles/360019833020-Download-and-install-Arduino-IDE).
- Permission to install applications (and a USB driver if needed). A managed
  school laptop may need an administrator's help.
- Internet access for the IDE, board package, libraries, and live flight data.
- A **USB-C cable that carries data**, with a connector that fits your Mac.
  A USB-C hub or USB-A adapter must also support data. A charge-only cable can
  light the screen but will never let you upload code.
- Several GB of free disk space for the ESP32 development tools.

The main instructions use macOS. Windows and Linux differences are included
where needed. No Terminal commands are required for the main build.

## Wi-Fi and a phone

Use a **2.4 GHz home Wi-Fi network** with a name and password, plus a phone
or another Wi-Fi device to open the setup page. The ESP32-S3 cannot join a
5 GHz-only network. A router using one name for both bands can work normally;
you do not need to rename your network just for this project.

School/work networks requiring a username, certificate, or browser sign-in
are outside this guide. The setup page supports a normal home network password.
Have that password ready before starting.

## Optional

- A nonconductive stand or case that fits this exact board and leaves the USB
  socket and buttons accessible. Keep the exposed circuit board off metal.
- A reliable **5 V USB power supply rated for at least 1 A**, to run it without
  the computer after programming. Power this build through USB; leave the
  board's relay terminals and other connectors unused.

**Ready to continue:** correct board, data cable, Mac, Wi-Fi password, and a
phone are available.

Next: [2. Install the Arduino IDE](02-install-arduino.md)
