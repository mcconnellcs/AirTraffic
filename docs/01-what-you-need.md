# 1. What you need

Total cost is about $25, and the only tool is a USB cable.

## The board

**ESP32-4848S040** — a 4-inch square touchscreen with an ESP32-S3 computer built
into the back. It is sold under several brand names (AITRIP, Guition, Sunton,
"ESP32 4.0 inch display"); they are all the same design. Look for these words in
the listing:

- 4.0 inch, **480 x 480**
- ESP32-S3
- **16MB flash** and **8MB PSRAM** (sometimes written "N16R8")
- Capacitive touch

Flip the board over: the model name `ESP32-4848S040` is printed on the back.
Some come with a version suffix like `4848S040C_I` — that's fine.

> **Why this board?** It has a big, sharp screen and lots of memory for the price,
> and hundreds of hobby projects already run on it, so help is easy to find.

## A USB cable

A **USB-C cable that carries data**. Many cheap cables only carry power for
charging; if your computer never sees the board, try a different cable first.

## A computer

Windows, Mac or Linux. You'll install the free Arduino IDE on it (next guide).

## Wi-Fi

The board needs a **2.4 GHz** Wi-Fi network. Most home routers have one, often
alongside a 5 GHz network with a similar name. The ESP32 cannot see 5 GHz
networks, so if your radar can't find your Wi-Fi, that's the first thing to check.

## Optional

- A stand or case. Search Printables or Thingiverse for "ESP32-4848S040 case"
  and there are several to 3D-print.
- A 5V USB power adapter, so the finished radar can live on a shelf without a
  computer.

Next: [2. Install the Arduino IDE](02-install-arduino.md)
