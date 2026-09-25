# 3. Flash it

"Flashing" means copying a program into the board's memory. We'll do it twice:
first a small test program to prove the screen works, then the real thing.

## Step 1 — Get the code

Either:

- Click the green **Code** button on the GitHub page, choose **Download ZIP**,
  and unzip it somewhere you'll find again, or
- if you know git: `git clone https://github.com/mcconnellcs/AirTraffic.git`

## Step 2 — Flash the BoardTest first

1. In the Arduino IDE open **File ▸ Open…** and pick
   `firmware/BoardTest/BoardTest.ino`.
2. Check the **Tools** menu settings from the last guide (especially PSRAM).
3. Click the **→ Upload** button (top left). The IDE compiles for a minute or
   two, then you'll see `Writing at 0x...` lines scroll past.
4. Look at the board. You should see:
   - four colour bars: red, green, blue, white
   - then a spinning green line inside rings, with an FPS number
   - touch the screen: an orange dot follows your finger

If you see all three, everything works. If not, jump to
[Troubleshooting](troubleshooting.md).

> **Tip:** open **Tools ▸ Serial Monitor** and set the speed to **115200 baud**.
> The board prints what it's doing. That window is your best friend when
> something goes wrong.

## Step 3 — Flash AirTraffic

1. **File ▸ Open…** ▸ `firmware/AirTraffic/AirTraffic.ino`. The IDE opens all
   the project's files as tabs.
2. Click **→ Upload** again. This one is bigger and takes a couple of minutes.
3. The board restarts and plays the start-up animation.

That's the flashing done. The board keeps the program forever (until you flash
something else), so you can unplug it from the computer and power it from any
USB adapter.

## Without the Arduino IDE (optional)

Every release on GitHub includes a ready-made `AirTraffic.ino.merged.bin`.
With Python installed you can flash it directly:

```
pip install esptool
esptool.py --chip esp32s3 --port COM5 --baud 460800 write_flash 0x0 AirTraffic.ino.merged.bin
```

(Replace `COM5` with your port.) This is handy for friends who only want the
radar and not the code.

Next: [4. First boot and Wi-Fi](04-first-boot.md)
