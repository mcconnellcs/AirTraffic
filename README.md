# AirTraffic

A personal flight radar for your desk, built on a $20 ESP32 touchscreen.
It shows every aircraft flying near you, live, with a proper radar sweep,
contrails and a card that tells you where each plane is going.

> Work in progress. The full step-by-step guide, photos and a ready-to-flash
> firmware file are coming in this README shortly.

- Board: **ESP32-4848S040** (4.0" 480x480 touch, ESP32-S3, 16 MB flash, 8 MB PSRAM)
- Toolchain: Arduino IDE 2.x with the ESP32 core 3.3.x
- Data: free community ADS-B feeds (adsb.lol, adsb.fi) and adsbdb.com. No accounts, no API keys.
- Wi-Fi: set up from your phone on first boot. No passwords in the code.

Inspired by [esp32flight](https://github.com/CrassusXY/esp32flight). MIT licensed.
