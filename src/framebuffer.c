#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "rpi/mailbox.h"
#include "framebuffer.h"
#include "rpi/v3d.h"

#include "Pi1MHz.h"

#define DEBUG_FB

#define BBC_X_RESOLUTION 1280
#define BBC_Y_RESOLUTION 1024

#define SCREEN_WIDTH    640
#define SCREEN_HEIGHT   480

//define how many bits per pixel

#define BPP8

#ifdef BPP32
#define SCREEN_DEPTH    32

#define ALPHA 0xFF000000
#define RED   0xFFFF0000
#define GREEN 0xFF00FF00
#define BLUE  0xFF0000FF

#endif

#ifdef BPP16
#define SCREEN_DEPTH    16

// R4 R3 R2 R1 R0 G5 G4 G3 G2 G1 G0 B4 B3 B2 B1 B0
#define ALPHA 0x0000
#define RED   0xF800
#define GREEN 0x07E0
#define BLUE  0x001F

#endif

#ifdef BPP8
#define SCREEN_DEPTH    8

#define ALPHA 0x0000
#define RED   0xE0
#define GREEN 0x1C
#define BLUE  0x03

#endif

#ifdef BPP4
#define SCREEN_DEPTH    4

#define ALPHA 0x0000
#define RED   0x8
#define GREEN 0x6
#define BLUE  0x1

#endif

#define D_RED   ((((RED & ~ALPHA)   >> 1) & RED)   | ALPHA)
#define D_GREEN ((((GREEN & ~ALPHA) >> 1) & GREEN) | ALPHA)
#define D_BLUE  ((((BLUE & ~ALPHA)  >> 1) & BLUE)  | ALPHA)

static uint32_t colour_table[] = {
   0x0000,                   // Black
   D_RED,                    // Dark Red
   D_GREEN,                  // Dark Green
   D_RED | D_GREEN,          // Dark Yellow
   D_BLUE,                   // Dark Blue
   D_RED | D_BLUE,           // Dark Magenta
   D_GREEN | D_BLUE,         // Dark Cyan
   D_RED | D_GREEN | D_BLUE, // Dark White

   0x0000,                   // Dark Black
   RED,                      // Red
   GREEN,                    // Green
   RED | GREEN,              // Yellow
   BLUE,                     // Blue
   RED | BLUE,               // Magenta
   GREEN | BLUE,             // Cyan
   RED | GREEN | BLUE        // White

};

// Character colour / cursor position
static uint8_t c_bg_col;
static uint8_t c_fg_col;
static uint8_t c_x_pos;
static uint8_t c_y_pos;

// Graphics colour / cursor position
static uint8_t g_bg_col;
static uint8_t g_fg_col;
static int16_t g_x_origin;
static int16_t g_x_pos;
static int16_t g_x_pos_last1;
static int16_t g_x_pos_last2;
static int16_t g_y_origin;
static int16_t g_y_pos;
static int16_t g_y_pos_last1;
static int16_t g_y_pos_last2;

// 6847 font data

#define cheight 12

static const uint8_t fontdata[] =
{
    0x00, 0x00, 0x00, 0x1c, 0x22, 0x02, 0x1a, 0x26, 0x26, 0x1c, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x08, 0x14, 0x22, 0x22, 0x3e, 0x22, 0x22, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x3c, 0x12, 0x12, 0x1c, 0x12, 0x12, 0x3c, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x1c, 0x22, 0x20, 0x20, 0x20, 0x22, 0x1c, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x3c, 0x12, 0x12, 0x12, 0x12, 0x12, 0x3c, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x3e, 0x20, 0x20, 0x38, 0x20, 0x20, 0x3e, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x3e, 0x20, 0x20, 0x38, 0x20, 0x20, 0x20, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x1c, 0x22, 0x20, 0x20, 0x26, 0x22, 0x1c, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x22, 0x22, 0x22, 0x3e, 0x22, 0x22, 0x22, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x1c, 0x08, 0x08, 0x08, 0x08, 0x08, 0x1c, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x02, 0x02, 0x02, 0x02, 0x02, 0x22, 0x1c, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x22, 0x24, 0x28, 0x30, 0x28, 0x24, 0x22, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x3e, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x22, 0x36, 0x2a, 0x2a, 0x22, 0x22, 0x22, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x22, 0x22, 0x32, 0x2a, 0x26, 0x22, 0x22, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x1c, 0x22, 0x22, 0x22, 0x22, 0x22, 0x1c, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x3c, 0x22, 0x22, 0x3c, 0x20, 0x20, 0x20, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x1c, 0x22, 0x22, 0x22, 0x2a, 0x24, 0x1a, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x3c, 0x22, 0x22, 0x3c, 0x28, 0x24, 0x22, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x1c, 0x22, 0x20, 0x1c, 0x02, 0x22, 0x1c, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x3e, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x1c, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x22, 0x22, 0x22, 0x14, 0x14, 0x08, 0x08, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x22, 0x22, 0x22, 0x22, 0x2a, 0x36, 0x22, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x22, 0x22, 0x14, 0x08, 0x14, 0x22, 0x22, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x22, 0x22, 0x14, 0x08, 0x08, 0x08, 0x08, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x3e, 0x02, 0x04, 0x08, 0x10, 0x20, 0x3e, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x1c, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1c, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x20, 0x10, 0x08, 0x04, 0x02, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x1c, 0x04, 0x04, 0x04, 0x04, 0x04, 0x1c, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x08, 0x1c, 0x2a, 0x08, 0x08, 0x08, 0x08, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x08, 0x10, 0x3e, 0x10, 0x08, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00, 0x08, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x14, 0x14, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x14, 0x14, 0x3e, 0x14, 0x3e, 0x14, 0x14, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x08, 0x1e, 0x28, 0x1c, 0x0a, 0x3c, 0x08, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x30, 0x32, 0x04, 0x08, 0x10, 0x26, 0x06, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x10, 0x28, 0x28, 0x10, 0x2a, 0x24, 0x1a, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x08, 0x08, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x04, 0x08, 0x10, 0x10, 0x10, 0x08, 0x04, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x10, 0x08, 0x04, 0x04, 0x04, 0x08, 0x10, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x08, 0x2a, 0x1c, 0x1c, 0x2a, 0x08, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x08, 0x08, 0x3e, 0x08, 0x08, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x20, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x02, 0x04, 0x08, 0x10, 0x20, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x1c, 0x22, 0x26, 0x2a, 0x32, 0x22, 0x1c, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x08, 0x18, 0x08, 0x08, 0x08, 0x08, 0x1c, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x1c, 0x22, 0x02, 0x1c, 0x20, 0x20, 0x3e, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x1c, 0x22, 0x02, 0x04, 0x02, 0x22, 0x1c, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x04, 0x0c, 0x14, 0x24, 0x3e, 0x04, 0x04, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x3e, 0x20, 0x3c, 0x02, 0x02, 0x22, 0x1c, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x1c, 0x20, 0x20, 0x3c, 0x22, 0x22, 0x1c, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x3e, 0x02, 0x04, 0x08, 0x10, 0x20, 0x20, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x1c, 0x22, 0x22, 0x1c, 0x22, 0x22, 0x1c, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x1c, 0x22, 0x22, 0x1e, 0x02, 0x02, 0x1c, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x08, 0x08, 0x10, 0x00,
    0x00, 0x00, 0x00, 0x04, 0x08, 0x10, 0x20, 0x10, 0x08, 0x04, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x3e, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x10, 0x08, 0x04, 0x02, 0x04, 0x08, 0x10, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x1c, 0x22, 0x02, 0x04, 0x08, 0x00, 0x08, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x08, 0x14, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x02, 0x1e, 0x22, 0x1e, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x20, 0x20, 0x2c, 0x32, 0x22, 0x32, 0x2c, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x22, 0x20, 0x22, 0x1c, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x02, 0x02, 0x1a, 0x26, 0x22, 0x26, 0x1a, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x22, 0x3e, 0x20, 0x1c, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x04, 0x0a, 0x08, 0x1c, 0x08, 0x08, 0x08, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x1a, 0x26, 0x22, 0x26, 0x1a, 0x02, 0x1c,
    0x00, 0x00, 0x00, 0x20, 0x20, 0x2c, 0x32, 0x22, 0x22, 0x22, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x08, 0x00, 0x18, 0x08, 0x08, 0x08, 0x1c, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x02, 0x00, 0x02, 0x02, 0x02, 0x02, 0x22, 0x1c, 0x00,
    0x00, 0x00, 0x00, 0x20, 0x20, 0x24, 0x28, 0x30, 0x28, 0x24, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x18, 0x08, 0x08, 0x08, 0x08, 0x08, 0x1c, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x2a, 0x2a, 0x2a, 0x2a, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x2c, 0x32, 0x22, 0x22, 0x22, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x22, 0x22, 0x22, 0x1c, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x22, 0x22, 0x22, 0x3c, 0x20, 0x20,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x1e, 0x22, 0x22, 0x22, 0x1e, 0x02, 0x02,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x2c, 0x32, 0x20, 0x20, 0x20, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x1e, 0x20, 0x1c, 0x02, 0x3c, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x10, 0x10, 0x38, 0x10, 0x10, 0x12, 0x0c, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x22, 0x22, 0x22, 0x26, 0x1a, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x22, 0x22, 0x22, 0x14, 0x08, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x22, 0x2a, 0x2a, 0x14, 0x14, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x22, 0x14, 0x08, 0x14, 0x22, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x22, 0x22, 0x22, 0x1e, 0x02, 0x1c, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x04, 0x08, 0x10, 0x3e, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x04, 0x08, 0x08, 0x10, 0x08, 0x08, 0x04, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x08, 0x08, 0x08, 0x00, 0x08, 0x08, 0x08, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x10, 0x08, 0x08, 0x04, 0x08, 0x08, 0x10, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x10, 0x2a, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x08, 0x00, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x08, 0x1c, 0x20, 0x20, 0x20, 0x1c, 0x08, 0x00,
    0x00, 0x00, 0x00, 0x0c, 0x12, 0x10, 0x38, 0x10, 0x10, 0x3e, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x22, 0x1c, 0x14, 0x1c, 0x22, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x22, 0x14, 0x08, 0x3e, 0x08, 0x3e, 0x08, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x08, 0x08, 0x08, 0x00, 0x08, 0x08, 0x08, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x1c, 0x20, 0x1c, 0x22, 0x1c, 0x02, 0x1c, 0x00, 0x00,
    0x14, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x3e, 0x41, 0x5d, 0x51, 0x51, 0x5d, 0x41, 0x3e, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x1c, 0x02, 0x1e, 0x22, 0x1e, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x0a, 0x14, 0x28, 0x14, 0x0a, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x02, 0x02, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x3e, 0x41, 0x5d, 0x55, 0x59, 0x55, 0x41, 0x3e, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x10, 0x28, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x08, 0x08, 0x3e, 0x08, 0x08, 0x00, 0x3e, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x18, 0x04, 0x08, 0x10, 0x1c, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x18, 0x04, 0x18, 0x04, 0x18, 0x00, 0x00, 0x00, 0x00,
    0x04, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x12, 0x12, 0x12, 0x12, 0x1c, 0x10, 0x20,
    0x00, 0x00, 0x00, 0x1a, 0x2a, 0x2a, 0x1a, 0x0a, 0x0a, 0x0a, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x18,
    0x00, 0x00, 0x00, 0x08, 0x18, 0x08, 0x08, 0x1c, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x1c, 0x22, 0x22, 0x22, 0x1c, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x28, 0x14, 0x0a, 0x14, 0x28, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x20, 0x20, 0x20, 0x22, 0x06, 0x0e, 0x02, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x20, 0x20, 0x20, 0x2e, 0x02, 0x04, 0x0e, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x70, 0x10, 0x70, 0x12, 0x76, 0x0e, 0x02, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x08, 0x00, 0x08, 0x08, 0x10, 0x12, 0x0c, 0x00, 0x00
};

static unsigned char* fb = NULL;
static unsigned char* fbbase = NULL;
static uint16_t width, height;

static int bpp, pitch;

static void fb_init_variables() {

    // Character colour / cursor position
   c_bg_col = 0;
   c_fg_col = 15;
   c_x_pos  = 0;
   c_y_pos  = 0;

   // Graphics colour / cursor position
   g_bg_col      = 0;
   g_fg_col      = 15;
   g_x_origin    = 0;
   g_x_pos       = 0;
   g_x_pos_last1 = 0;
   g_x_pos_last2 = 0;
   g_y_origin    = 0;
   g_y_pos       = 0;
   g_y_pos_last1 = 0;
   g_y_pos_last2 = 0;
}

static void fb_putpixel(int x, int y, unsigned int colour) {
   x = ((x + g_x_origin) * SCREEN_WIDTH)  / BBC_X_RESOLUTION;
   y = ((y + g_y_origin) * SCREEN_HEIGHT) / BBC_Y_RESOLUTION;
   if (x < 0  || x > SCREEN_WIDTH - 1) {
      return;
   }
   if (y < 0 || y > SCREEN_HEIGHT - 1) {
      return;
   }
#ifdef BPP32
   *(uint32_t *)(fb + (SCREEN_HEIGHT - y - 1) * pitch + x * 4) = colour_table[colour];
#endif
#ifdef BPP16
   *(uint16_t *)(fb + (SCREEN_HEIGHT - y - 1) * pitch + x * 2) = colour_table[colour];
#endif
#ifdef BPP8
   *(uint8_t *)(fb + (SCREEN_HEIGHT - y - 1) * pitch + x * 1) = colour_table[colour];
#endif
#ifdef BPP4
   uint8_t *fbptr = (uint8_t *)(fb + (SCREEN_HEIGHT - y - 1) * pitch + x * 1);

   if (~x&1)
      {*fbptr = ((*fbptr) &0x0F) | colour_table[colour]<<4;}
   else
      {*fbptr = ((*fbptr) &0xF0) | colour_table[colour];}
   return;
#endif
}

// NB these functions can over run the buffer
static void memmovequick( void* addr_dst, const void * addr_src, size_t length)
{
  uint32_t * dst = (uint32_t *) addr_dst;
  const uint32_t * src = (const uint32_t *) addr_src;
  uint32_t temp = length;
  length = (length >>2)>>2;
  if ((length<<4)<(temp)) length +=1;
  for ( ; length; length--) {
    uint32_t a = *src++;
    uint32_t b = *src++;
    uint32_t c = *src++;
    uint32_t d = *src++;
    *dst++ = a;
    *dst++ = b;
    *dst++ = c;
    *dst++ = d;
  }
}

// Value is treated as a 32 bit word which is wrong
static void memsetquick( void * ptr, int value, size_t num )
{
  uint32_t * dst = (uint32_t *) ptr;
  uint32_t a=value;
  uint32_t length = num;
  length = (length >>2);
  // not much point here of doing more than 4 bytes as the write buffer will improve
  // peformance
  for ( ; length; length--) {
    *dst++ = a;
  }
}

static void fb_scroll() {

//RPI_SetGpioHi(TEST_PIN);
#if 0
   static uint32_t viewport = 0;
   viewport++;
   fb = fb + cheight * pitch;
   if (viewport>40)
   {
      viewport = 0;
      fb = fbbase;
   }
   RPI_PropertySetWord( TAG_SET_VIRTUAL_OFFSET , 0 , cheight*viewport );
   memsetquick(fbbase + (height - cheight) * pitch, 0, cheight * pitch);
#else
   memmovequick(fb, fb + cheight * pitch, (height - cheight) * pitch);
   memsetquick(fb + (height - cheight) * pitch, 0, cheight * pitch);
#endif

//RPI_SetGpioLo(TEST_PIN);
}

static void fb_clear() {
   // initialize the cursor positions, origin, colours, etc.
   // TODO: clearing all of these may not be strictly correct
   fb_init_variables();
   // clear frame buffer *** bug here memset value is a byte so 32bpp doesn't work
   memsetquick((void *)fb, colour_table[c_bg_col], height * pitch);
}

static void fb_cursor_left() {
   if (c_x_pos != 0) {
      c_x_pos--;
   } else {
      c_x_pos = 79;
   }
}

static void fb_cursor_right() {
   if (c_x_pos < 79) {
      c_x_pos++;
   } else {
      c_x_pos = 0;
   }
}

static void fb_cursor_up() {
   if (c_y_pos != 0) {
      c_y_pos--;
   } else {
      c_y_pos = 39;
   }
}

static void fb_cursor_down() {
   if (c_y_pos < 39) {
      c_y_pos++;
   } else {
      fb_scroll();
   }
}

static void fb_cursor_col0() {
   c_x_pos = 0;
}

static void fb_cursor_home() {
   c_x_pos = 0;
   c_y_pos = 0;
}

static void fb_cursor_next() {
   if (c_x_pos < 79) {
      c_x_pos++;
   } else {
      c_x_pos = 0;
      fb_cursor_down();
   }
}

// Implementation of Bresenham's line drawing algorithm from here:
// http://tech-algorithm.com/articles/drawing-line-using-bresenham-algorithm/
static void fb_draw_line(int x,int y,int x2, int y2, unsigned int color) {
   int i;
   int w = x2 - x;
   int h = y2 - y;
   int dx1 = 0, dy1 = 0, dx2 = 0, dy2 = 0;
   if (w < 0) dx1 = -1 ; else if (w > 0) dx1 = 1;
   if (h < 0) dy1 = -1 ; else if (h > 0) dy1 = 1;
   if (w < 0) dx2 = -1 ; else if (w > 0) dx2 = 1;
   int longest = abs(w);
   int shortest = abs(h);
   if (!(longest > shortest)) {
      longest = abs(h);
      shortest = abs(w);
      if (h < 0) dy2 = -1 ; else if (h > 0) dy2 = 1;
      dx2 = 0;
   }
   int numerator = longest >> 1 ;
   for (i = 0; i <= longest; i++) {
      fb_putpixel(x, y, color);
      numerator += shortest;
      if (!(numerator < longest)) {
         numerator -= longest;
         x += dx1;
         y += dy1;
      } else {
         x += dx2;
         y += dy2;
      }
   }
}

static void fb_fill_triangle(int x, int y, int x2, int y2, int x3, int y3, unsigned int colour) {
   x = ((x + g_x_origin) * SCREEN_WIDTH)  / BBC_X_RESOLUTION;
   y = ((y + g_y_origin) * SCREEN_HEIGHT) / BBC_Y_RESOLUTION;
   x2 = ((x2 + g_x_origin) * SCREEN_WIDTH)  / BBC_X_RESOLUTION;
   y2 = ((y2 + g_y_origin) * SCREEN_HEIGHT) / BBC_Y_RESOLUTION;
   x3 = ((x3 + g_x_origin) * SCREEN_WIDTH)  / BBC_X_RESOLUTION;
   y3 = ((y3 + g_y_origin) * SCREEN_HEIGHT) / BBC_Y_RESOLUTION;
   // Flip y axis
   y = SCREEN_HEIGHT - 1 - y;
   y2 = SCREEN_HEIGHT - 1 - y2;
   y3 = SCREEN_HEIGHT - 1 - y3;
   colour = colour_table[colour];
   v3d_draw_triangle(x, y, x2, y2, x3, y3, colour);
}


static uint32_t fb_get_address() {
   return (uint32_t) fb;
}

static uint32_t fb_get_width() {
   return width;
}

static uint32_t fb_get_height() {
   return height;
}

static uint32_t fb_get_bpp() {
   return bpp;
}

static uint32_t palette= PALETTE_DEFAULT;
static uint32_t active = 0;

static void update_palette() {
   int m;
   uint32_t palette_data[258];
   uint32_t num_colours = (bpp == 8) ? 256 : 16;
   uint32_t j = 2;
   palette_data[0] = 0;                        // Offset to first colour
   palette_data[1] = num_colours;              // Number of colours
   for (uint32_t i = 0; i < num_colours; i++) {
      int r = (i & 1) ? 255 : 0;
      int g = (i & 2) ? 255 : 0;
      int b = (i & 4) ? 255 : 0;
      switch (palette) {
      case PALETTE_INVERSE:
         r = 255 - r;
         g = 255 - g;
         b = 255 - b;
         break;
      case PALETTE_MONO1:
         m = 0.299 * r + 0.587 * g + 0.114 * b;
         r = m; g = m; b = m;
         break;
      case PALETTE_MONO2:
         m = (i & 7) * 255 / 7;
         r = m; g = m; b = m;
         break;
      case PALETTE_RED:
         m = (i & 7) * 255 / 7;
         r = m; g = 0; b = 0;
         break;
      case PALETTE_GREEN:
         m = (i & 7) * 255 / 7;
         r = 0; g = m; b = 0;
         break;
      case PALETTE_BLUE:
         m = (i & 7) * 255 / 7;
         r = 0; g = 0; b = m;
         break;
      case PALETTE_NOT_RED:
         r = 0;
         g = (i & 3) * 255 / 3;
         b = ((i >> 2) & 1) * 255;
         break;
      case PALETTE_NOT_GREEN:
         r = (i & 3) * 255 / 3;
         g = 0;
         b = ((i >> 2) & 1) * 255;
         break;
      case PALETTE_NOT_BLUE:
         r = ((i >> 2) & 1) * 255;
         g = (i & 3) * 255 / 3;
         b = 0;
         break;
      case PALETTE_ATOM_COLOUR_NORMAL:
         // In the Atom CPLD, colour bit 3 indicates additional colours
         //  8 = 1000 = normal orange
         //  9 = 1001 = bright orange
         // 10 = 1010 = dark green text background
         // 11 = 1011 = dark orange text background
         if (i & 8) {
            if ((i & 7) == 0) {
               // orange
               r = 160; g = 80; b = 0;
            } else if ((i & 7) == 1) {
               // bright orange
               r = 255; g = 127; b = 0;
            } else {
               // otherwise show as black
               r = g = b = 0;
            }
         }
         break;
      case PALETTE_ATOM_COLOUR_EXTENDED:
         // In the Atom CPLD, colour bit 3 indicates additional colours
         if (i & 8) {
            if ((i & 7) == 0) {
               // orange
               r = 160; g = 80; b = 0;
            } else if ((i & 7) == 1) {
               // bright orange
               r = 255; g = 127; b = 0;
            } else if ((i & 7) == 2) {
               // dark green
               r = 0; g = 31; b = 0;
            } else if ((i & 7) == 3) {
               // dark orange
               r = 31; g = 15; b = 0;
            } else {
               // otherwise show as black
               r = g = b = 0;
            }
         }
         break;
      case PALETTE_ATOM_COLOUR_ACORN:
         // In the Atom CPLD, colour bit 3 indicates additional colours
         if (i & 8) {
            if ((i & 6) == 0) {
               // orange => red
               r = 255; g = 0; b = 0;
            } else {
               // otherwise show as black
               r = g = b = 0;
            }
         }
         break;
      case PALETTE_ATOM_MONO:
         m = 0;
         switch (i) {
         case 3: // yellow
         case 7: // white (buff)
         case 9: // bright orange
            // Y = WH (0.42V)
            m = 255;
            break;
         case 2: // green
         case 5: // magenta
         case 6: // cyan
         case 8: // normal orange
            // Y = WM (0.54V)
            m = 255 * (72 - 54) / (72 - 42);
            break;
         case 1: // red
         case 4: // blue
            // Y = WL (0.65V)
            m = 255 * (72 - 65) / (72 - 42);
            break;
         default:
            // Y = BL (0.72V)
            m = 0;
         }
         r = g = b = m;
         break;
      }
      if (active) {
         if (i >= (num_colours >> 1)) {
            palette_data[j++] = 0xFFFFFFFF;
         } else {
            r >>= 1; g >>= 1; b >>= 1;
            palette_data[j++] = 0xFF000000 | (b << 16) | (g << 8) | r;
         }
      } else {
         palette_data[j++] = 0xFF000000 | (b << 16) | (g << 8) | r;
      }
   }

   // Flush the previous swapBuffer() response from the GPU->ARM mailbox

   RPI_PropertySetBuffer(TAG_SET_PALETTE, palette_data, j );
}


#define NORMAL    0
#define IN_VDU17  1
#define IN_VDU18  2
#define IN_VDU25  3
#define IN_VDU29  4

static void update_g_cursors(int16_t x, int16_t y) {
   g_x_pos_last2 = g_x_pos_last1;
   g_x_pos_last1 = g_x_pos;
   g_x_pos       = x;
   g_y_pos_last2 = g_y_pos_last1;
   g_y_pos_last1 = g_y_pos;
   g_y_pos       = y;
}

static void fb_draw_character(int c, int invert, int eor) {
#ifdef BPP4
   unsigned char temp=0;
#endif

   // Map the character to a section of the 6847 font data
   c *= cheight;

   if(invert) invert = 0xFF ;
   // Copy the character into the frame buffer
   for (uint32_t i = 0; i < cheight; i++) {
      int data = fontdata[c + i] ^ invert;
#ifdef BPP32
      uint32_t *fbptr = fb + c_x_pos * bpp + (c_y_pos * cheight + i) * pitch;
#endif
#ifdef BPP16
      uint16_t *fbptr = fb + c_x_pos * bpp + (c_y_pos * cheight + i) * pitch;
#endif
#ifdef BPP8
      uint8_t *fbptr = fb + c_x_pos * bpp + (c_y_pos * cheight + i) * pitch;
#endif
#ifdef BPP4
      uint8_t *fbptr = fb + c_x_pos * bpp + (c_y_pos * cheight + i) * pitch;
#endif

      uint32_t c_fgol = colour_table[c_fg_col];
      uint32_t c_bgol = colour_table[c_bg_col];

      for (uint32_t j = 0; j < 8; j++) {
         int col = (data & 0x80) ? c_fgol : c_bgol;

         if (eor) {
            *fbptr++ ^= col;
         } else {
            *fbptr++ = col;
         }

#ifdef BPP4
         if (eor) {
            *fbptr ^= col; // FIX 4 bit EOR
         } else {
           if (j & 1)
           {
             *(uint8_t *)(fb + c_x_pos * bpp+(0+(j>>1)) + (c_y_pos * cheight + i) * pitch) = (temp<<4) | col;
             fbptr += 1;
           } else
           {
             temp = col;
           }
         }
#endif
         data <<= 1;
      }
   }
}

void fb_writec(int c) {
   int invert;
   static uint8_t g_mode;
//   static uint8_t g_plotmode; // not currently used
   static int state = NORMAL;
   static int count = 0;
   static int16_t x_tmp;
   static int16_t y_tmp;

   if (state == IN_VDU17) {
      state = NORMAL;
      if (c & 128) {
         c_bg_col = c & 15;
#ifdef DEBUG_VDU
         printf("bg = %d\r\n", c_bg_col);
#endif
      } else {
         c_fg_col = c & 15;
#ifdef DEBUG_VDU
         printf("fg = %d\r\n", c_fg_col);
#endif
      }
      return;

   } else if (state == IN_VDU18) {
      switch (count) {
      case 0:
//         g_plotmode = c;
         break;
      case 1:
         if (c & 128) {
            g_bg_col = c & 15;
         } else {
            g_fg_col = c & 15;
         }
         break;
      }
      count++;
      if (count == 2) {
         state = NORMAL;
      }
      return;

   } else if (state == IN_VDU25) {
      switch (count) {
      case 0:
         g_mode = c;
         break;
      case 1:
         x_tmp = c;
         break;
      case 2:
         x_tmp |= c << 8;
         break;
      case 3:
         y_tmp = c;
         break;
      case 4:
         y_tmp |= c << 8;
#ifdef DEBUG_VDU
         printf("plot %d %d %d\r\n", g_mode, x_tmp, y_tmp);
#endif

         if (g_mode & 4) {
            // Absolute position X, Y.
            update_g_cursors(x_tmp, y_tmp);
         } else {
            // Relative to the last point.
            update_g_cursors(g_x_pos + x_tmp, g_y_pos + y_tmp);
         }


         int col;
         switch (g_mode & 3) {
         case 0:
            col = -1;
            break;
         case 1:
            col = g_fg_col;
            break;
         case 2:
            col = 15 - g_fg_col;
            break;
         case 3:
            col = g_bg_col;
            break;
         }

         if (col >= 0) {
            if (g_mode < 32) {
               // Draw a line
               fb_draw_line(g_x_pos_last1, g_y_pos_last1, g_x_pos, g_y_pos, col);
            } else if (g_mode >= 64 && g_mode < 72) {
               // Plot a single pixel
               fb_putpixel(g_x_pos, g_y_pos, g_fg_col);
            } else if (g_mode >= 80 && g_mode < 88) {
               // Fill a triangle
               fb_fill_triangle(g_x_pos_last2, g_y_pos_last2, g_x_pos_last1, g_y_pos_last1, g_x_pos, g_y_pos, col);
            }
         }
      }
      count++;
      if (count == 5) {
         state = NORMAL;
      }
      return;

   } else if (state == IN_VDU29) {
      switch (count) {
      case 0:
         x_tmp = c;
         break;
      case 1:
         x_tmp |= c << 8;
         break;
      case 2:
         y_tmp = c;
         break;
      case 3:
         y_tmp |= c << 8;
         g_x_origin = x_tmp;
         g_y_origin = y_tmp;
#ifdef DEBUG_VDU
         printf("graphics origin %d %d\r\n", g_x_origin, g_y_origin);
#endif
      }
      count++;
      if (count == 4) {
         state = NORMAL;
      }
      return;
   }

   switch(c) {

   case 8:
      fb_draw_character(32, 1, 1);
      fb_cursor_left();
      fb_draw_character(32, 1, 1);
      break;

   case 9:
      fb_draw_character(32, 1, 1);
      fb_cursor_right();
      fb_draw_character(32, 1, 1);
      break;

   case 10:
      fb_draw_character(32, 1, 1);
      fb_cursor_down();
      fb_draw_character(32, 1, 1);
      break;

   case 11:
      fb_draw_character(32, 1, 1);
      fb_cursor_up();
      fb_draw_character(32, 1, 1);
      break;

   case 12:
      fb_clear();
      fb_draw_character(32, 1, 1);
      break;

   case 13:
      fb_draw_character(32, 1, 1);
      fb_cursor_col0();
      fb_draw_character(32, 1, 1);
      break;

   case 17:
      state = IN_VDU17;
      count = 0;
      return;

   case 18:
      state = IN_VDU18;
      count = 0;
      return;

   case 25:
      state = IN_VDU25;
      count = 0;
      return;

   case 29:
      state = IN_VDU29;
      count = 0;
      return;

   case 30:
      fb_draw_character(32, 1, 1);
      fb_cursor_home();
      fb_draw_character(32, 1, 1);
      break;

   case 127:
      fb_draw_character(32, 1, 1);
      fb_cursor_left();
      fb_draw_character(32, 1, 0);
      break;

   default:

      // Convert c to index into 6847 character generator ROM
      // chars 20-3F map to 20-3F
      // chars 40-5F map to 00-1F
      // chars 60-7F map to 40-5F
      invert = c >= 0x80;
      c &= 0x7f;
      if (c < 0x20) {
         return;
      } else if (c >= 0x40 && c < 0x5F) {
         c -= 0x40;
      } else if (c >= 0x60) {
         c -= 0x20;
      }

      // Erase the cursor
      fb_draw_character(32, 1, 1);

      // Draw the next character
      fb_draw_character(c, invert, 0);

      // Advance the drawing position
      fb_cursor_next();

      // Draw the cursor
      fb_draw_character(32, 1, 1);
   }
}

static void fb_writes(char *string) {
   while (*string) {
      fb_writec(*string++);
   }
}

static void fb_initialize() {

    rpi_mailbox_property_t *mp;

    /* Initialise a framebuffer... */
    RPI_PropertyInit();
    RPI_PropertyAddTag(TAG_ALLOCATE_BUFFER , 64 );
    RPI_PropertyAddTag(TAG_SET_PHYSICAL_SIZE, SCREEN_WIDTH, SCREEN_HEIGHT );
    RPI_PropertyAddTag(TAG_SET_VIRTUAL_SIZE, SCREEN_WIDTH, SCREEN_HEIGHT * 2 );
    RPI_PropertyAddTag(TAG_SET_DEPTH, SCREEN_DEPTH );
    RPI_PropertyProcess();

    if( ( mp = RPI_PropertyGet( TAG_ALLOCATE_BUFFER  ) ) )
    {
        fb = (unsigned char*)(mp->data.buffer_32[0] &0x1FFFFFFF);
        fbbase = fb;
#ifdef DEBUG_FB
        printf( "Framebuffer address: %8.8X\r\n", (unsigned int)fb );
#endif
    }

    if( ( mp = RPI_PropertyGetWord( TAG_GET_PHYSICAL_SIZE , 0) ) )
    {
        width = mp->data.buffer_32[0];
        height = mp->data.buffer_32[1];
#ifdef DEBUG_FB
        printf( "Initialised Framebuffer: %dx%d ", width, height );
#endif
    }

    if( ( mp = RPI_PropertyGetWord( TAG_GET_DEPTH , 0) ) )
    {
        bpp = mp->data.buffer_32[0];
#ifdef DEBUG_FB
        printf( "%dbpp\r\n", bpp );
#endif
    }

    if( ( mp = RPI_PropertyGetWord( TAG_GET_PITCH , 0) ) )
    {
        pitch = mp->data.buffer_32[0];
#ifdef DEBUG_FB
        printf( "Pitch: %d bytes\r\n", pitch );
#endif
    }

    // On the Pi 2/3 the mailbox returns the address with bits 31..30 set, which is wrong
    fb = (unsigned char *)(((unsigned int) fb) & 0x3fffffff);

    update_palette();

    // Change bpp from bits to bytes
  //  bpp >>= 3;

    fb_clear();

    fb_writes("\r\n\r\nAcorn MOS\r\n\r\nPi1MHz\r\n\r\nBASIC\r\n\r\n>");

#ifdef BPP32
       for (int y = 0; y < 16; y++) {
          uint32_t *fbptr = (uint32_t *) (fb + pitch * y);
          for (int col = 23; col >= 0; col--) {
             for (int x = 0; x < 8; x++) {
                *fbptr++ = 1 << col;
             }
          }
       }
#endif
#ifdef BPP16
       for (int y = 0; y < 16; y++) {
          uint16_t *fbptr = (uint16_t *) (fb + pitch * y);
          for (int col = 15; col >= 0; col--) {
             for (int x = 0; x < 8; x++) {
                *fbptr++ = 1 << col;
             }
          }
       }
#endif
#ifdef BPP8
       for (int y = 0; y < 16; y++) {
          uint8_t *fbptr = (uint8_t *) (fb + pitch * y);
          for (int col = 15; col >= 0; col--) {
             for (int x = 0; x < 8; x++) {
                *fbptr++ = 1 << col;
             }
          }
       }
#endif

}

#define QSIZE 4096

volatile int wp = 0;
static int rp = 0;

unsigned char vdu_queue[QSIZE];

static void fb_emulator_vdu(unsigned int gpio)
{
   vdu_queue[wp] = GET_DATA(gpio);
   wp = (wp + 1) & (QSIZE - 1);
}

static void fb_emulator_poll()
{
   if (rp != wp) {
      fb_writec(vdu_queue[rp]);
      rp = (rp + 1) & (QSIZE - 1);
   }
}

void fb_emulator_init(uint8_t instance)
{
  fb_initialize();

  v3d_initialize(fb_get_address(), fb_get_width(), fb_get_height(), fb_get_bpp());

  Pi1MHz_Register_Memory(WRITE_FRED, 0xd0, fb_emulator_vdu);

  Pi1MHz_Register_Poll(fb_emulator_poll);
}
