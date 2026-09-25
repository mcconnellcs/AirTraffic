// =============================================================================
//  board_config.h  —  How the screen and touch chip are wired on YOUR board
// =============================================================================
//
//  Board: "ESP32-4848S040" (sold by AITRIP, Guition, Sunton and others)
//         ESP32-S3 + 4.0" 480x480 IPS screen (ST7701S) + GT911 touch
//
//  You normally never need to edit this file. It tells the LovyanGFX
//  graphics library which ESP32 pins connect to which wire on the screen.
//
//  How does a 480x480 screen get its pictures?
//    The ESP32 sends every pixel's color over 16 wires at once
//    (5 red + 6 green + 5 blue = "RGB565", 65,536 colors).
//    Four more wires (PCLK, HSYNC, VSYNC, DE) keep time, like a metronome,
//    so the screen knows where each pixel goes.
// =============================================================================
#pragma once

#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>

// Screen size in pixels
constexpr int SCREEN_W = 480;
constexpr int SCREEN_H = 480;

// Backlight pin (see backlight.h for how it is driven)
constexpr int PIN_BACKLIGHT = 38;

// Which way round the board is mounted. The picture is turned to match:
//   0 = USB socket at the bottom (as the board is printed)
//   1 = picture turned 90 degrees clockwise
//   2 = upside down
//   3 = picture turned 90 degrees counter-clockwise
// Touch is turned the same way. Change this if your stand holds it sideways.
constexpr int SCREEN_ROTATION = 3;

// The screen driver, plus one extra door: the strip copier (ui_canvas.cpp)
// writes straight into the screen's picture memory, row by row, which is
// faster than going through the library for every block.
struct Panel : public lgfx::Panel_ST7701_guition_esp32_4848S040 {
  uint16_t* row(int y) { return reinterpret_cast<uint16_t*>(_lines_buffer[y]); }
};

class LGFX : public lgfx::LGFX_Device {
  lgfx::Bus_RGB      bus_;
  Panel              panel_;
  lgfx::Touch_GT911  touch_;

 public:
  Panel& panel() { return panel_; }

  LGFX() {
    {  // --- The picture area ---
      auto cfg = panel_.config();
      cfg.memory_width  = SCREEN_W;
      cfg.memory_height = SCREEN_H;
      cfg.panel_width   = SCREEN_W;
      cfg.panel_height  = SCREEN_H;
      cfg.offset_x = 0;
      cfg.offset_y = 0;
      panel_.config(cfg);
    }
    {  // --- 3 wires used ONCE at start-up to send setup commands to the screen ---
      auto cfg = panel_.config_detail();
      cfg.pin_cs   = 39;
      cfg.pin_sclk = 48;
      cfg.pin_mosi = 47;
      panel_.config_detail(cfg);
    }
    {  // --- The 16 color wires + 4 timing wires ---
      auto cfg = bus_.config();
      cfg.panel = &panel_;
      // Blue (5 wires)
      cfg.pin_d0  = 4;   cfg.pin_d1  = 5;   cfg.pin_d2  = 6;
      cfg.pin_d3  = 7;   cfg.pin_d4  = 15;
      // Green (6 wires — our eyes are most sensitive to green!)
      cfg.pin_d5  = 8;   cfg.pin_d6  = 20;  cfg.pin_d7  = 3;
      cfg.pin_d8  = 46;  cfg.pin_d9  = 9;   cfg.pin_d10 = 10;
      // Red (5 wires)
      cfg.pin_d11 = 11;  cfg.pin_d12 = 12;  cfg.pin_d13 = 13;
      cfg.pin_d14 = 14;  cfg.pin_d15 = 0;
      // Timing wires
      cfg.pin_henable = 18;  // DE    = "this pixel is real"
      cfg.pin_vsync   = 17;  // VSYNC = "new picture starts"
      cfg.pin_hsync   = 16;  // HSYNC = "new row starts"
      cfg.pin_pclk    = 21;  // PCLK  = "next pixel, please"
      cfg.freq_write  = 14000000;  // 14 million pixels per second

      cfg.hsync_polarity    = 0;
      cfg.hsync_front_porch = 10;
      cfg.hsync_pulse_width = 8;
      cfg.hsync_back_porch  = 50;
      cfg.vsync_polarity    = 0;
      cfg.vsync_front_porch = 10;
      cfg.vsync_pulse_width = 8;
      cfg.vsync_back_porch  = 20;
      cfg.pclk_idle_high    = 0;
      cfg.de_idle_high      = 1;
      bus_.config(cfg);
    }
    panel_.setBus(&bus_);

    {  // --- GT911 touch chip (talks over I2C: 2 wires, SDA + SCL) ---
      auto cfg = touch_.config();
      cfg.x_min = 0;  cfg.x_max = SCREEN_W - 1;
      cfg.y_min = 0;  cfg.y_max = SCREEN_H - 1;
      cfg.pin_int  = -1;   // not connected on this board
      cfg.pin_rst  = -1;   // not connected on this board
      cfg.bus_shared = false;
      cfg.offset_rotation = 0;
      cfg.i2c_port = 1;
      cfg.pin_sda  = 19;
      cfg.pin_scl  = 45;
      cfg.freq     = 400000;
      cfg.i2c_addr = 0x5D;  // some boards answer on 0x14 instead
      touch_.config(cfg);
    }
    panel_.setTouch(&touch_);

    setPanel(&panel_);
  }
};
