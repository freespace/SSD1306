/**
 * Derived from:
 *
 * https://github.com/adafruit/Adafruit_SSD1306/blob/master/Adafruit_SSD1306.h
 *
 * This is developed specifically for my programmable watch project, and as
 * such no effort is made to support other display configurations other than
 * what I have on hand.
 *
 * freespace@gmail.com 2015-06-13
 */

#ifndef _SSD1306_H_
#define _SSD1306_H_

#include "arduino.h"
#include "SGL.h"

#define SSD1306_128_64

#if defined SSD1306_128_64
  #define SSD1306_LCDWIDTH                  128
  #define SSD1306_LCDHEIGHT                 64
#endif

#define SSD1306_SETCONTRAST                 0x81
#define SSD1306_DISPLAYALLON_RESUME         0xA4
#define SSD1306_DISPLAYALLON                0xA5
#define SSD1306_NORMALDISPLAY               0xA6
#define SSD1306_INVERTDISPLAY               0xA7
#define SSD1306_DISPLAYOFF                  0xAE
#define SSD1306_DISPLAYON                   0xAF

#define SSD1306_SETDISPLAYOFFSET            0xD3
#define SSD1306_SETCOMPINS                  0xDA

#define SSD1306_SETVCOMDETECT               0xDB

#define SSD1306_SETDISPLAYCLOCKDIV          0xD5
#define SSD1306_SETPRECHARGE                0xD9

#define SSD1306_SETMULTIPLEX                0xA8

#define SSD1306_SETLOWCOLUMN                0x00
#define SSD1306_SETHIGHCOLUMN               0x10

#define SSD1306_SETSTARTLINE                0x40

#define SSD1306_MEMORYMODE                  0x20
#define SSD1306_COLUMNADDR                  0x21
#define SSD1306_PAGEADDR                    0x22

#define SSD1306_COMSCANINC                  0xC0
#define SSD1306_COMSCANDEC                  0xC8

#define SSD1306_SEGREMAP                    0xA0

#define SSD1306_CHARGEPUMP                  0x8D

#define SSD1306_EXTERNALVCC                 0x1
#define SSD1306_SWITCHCAPVCC                0x2

// Scrolling #defines
#define SSD1306_ACTIVATE_SCROLL             0x2F
#define SSD1306_DEACTIVATE_SCROLL           0x2E
#define SSD1306_SET_VERTICAL_SCROLL_AREA    0xA3
#define SSD1306_RIGHT_HORIZONTAL_SCROLL     0x26
#define SSD1306_LEFT_HORIZONTAL_SCROLL      0x27
#define SSD1306_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL 0x29
#define SSD1306_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL 0x2A

// So for RGB versions, each byte maps to a pixel exactly, and we can forego
// having a screen buffer. But for monochrome versions, each byte updates
// 8 pixels, and we only have byte level addressing. Therefore we must use
// buffers in order to be able to change a pixel without destroying the other
// 7 pixels.
//
// However memory is precious. A 128x64 memory buffer needs 128*64/8 = 1 KB of
// RAM. The ATMEGA32U4 only has 2.5 KB to spare, and we want to reserve some
// of it for the stack.
//
// One way to do this is the idea of an update region. Most updates only
// affect a small part of the screen. We create a buffer that can hold pixel
// for say, 1/4 of the screen, and all drawPixels in that region will be
// cached until end of drawing, and the whole region is written to the
// display.
//
// This method runs into the issue of region alignment. The above method makes
// it difficult to draw across the region boundary, especially for centred
// text, as the natural division of the screen into 4 sections will result in
// a cross shaped boundary in the middle.
//
// One way to resolve this is to recognise that most of our drawing for my
// watch project is essentially line based, and the base font size has
// a height of 8 pixels, and I use at most 2x the font size, so 16 pixels.
// In otherwords, a region that is 4 bytes high will accomodate my
// line-drawing needs as long as I draw one line at a time.
//
// So say we use 128x2 =256 byte buffer. This lets us draw non-overlapping
// 128x16 regions that are ALIGNED to the LED's memory. By aligned, it means
// that if you attempt to the buffer starting at say, (10,10), you WILL trash
// 2 pixels to the left and above the origin. The consequence then is that
// if you draw 2 lines using 2 regions, those 2 lines will be separated by no
// less than 8 pixels.
//
// This also means that if we draw 2 lines in the same region, the lines will
// be right on top of each other.
//
// So lets relax things somewhat, and use 128 x 3 = 384 bytes. This is awkward
// because it doesn't align nicely with the number of rows (64/(3*8) is not an
// integer). If we go to 128 x 4 = 512 bytes then we have the issue of
// a boundary splitting the middle of the screen!
//
// One saving grace is the fact this library doesn't need to be general
// purpose. If I plan out the watch UI, I can segment the screen into specific
// aligned regions for update, and tailor the update buffer accordingly. This
// will however require some foresight.
//
// After delibration, buffer space that can update half the screen at once
// will be made available, and this buffer space can be copied to any arbitary
// aligned display memory.

#define SSD1306_BUF_WIDTH       (128)
#define SSD1306_BUF_HEIGHT      (64/(2*8))
#define SSD1306_BUF_SIZE        (SSD1306_BUF_WIDTH*SSD1306_BUF_HEIGHT)
// XXX this is only here for backward compat, remove me when able
#define RGB(R,G,B)                  (((R>>3)<<11) | ((G>>2)<<5) | (B>>3))
enum Color{
    COLOR_BLACK     = RGB(  0,  0,  0), // black
    COLOR_GREY      = RGB(192,192,192), // grey
    COLOR_WHITE     = RGB(255,255,255), // white
    COLOR_RED       = RGB(255,  0,  0), // red
    COLOR_PINK      = RGB(255,192,203), // pink
    COLOR_YELLOW    = RGB(255,255,  0), // yellow
    COLOR_GOLDEN    = RGB(255,215,  0), // golden
    COLOR_BROWN     = RGB(128, 42, 42), // brown
    COLOR_BLUE      = RGB(  0,  0,255), // blue
    COLOR_CYAN      = RGB(  0,255,255), // cyan
    COLOR_GREEN     = RGB(  0,255,  0), // green
    COLOR_PURPLE    = RGB(160, 32,240), // purple
};

enum {
  SSD1306_ERROR_NOT_MULTIPLE_OF_8 = 1,
  SSD1306_ERROR_REGION_TOOLARGE,
  SSD1306_ERROR_UNKNOWN_REGION,
};

typedef enum {
  // allocates the buffer with its origin at (0,0) with same width as the
  // screen and as high as buffer size allows.
  SSD1306_REGION_TOP,

  // allocates the buffer such that the centre of the buffer is at screen
  // centre, as wide as the screen, and as high as buffer size allows.
  SSD1306_REGION_MID,

  // allocates the buffer so its bottom-right corner is also the bottom-right
  // corner of the screen, as wide as the screen, and as high as buffer size
  // allows.
  SSD1306_REGION_BOT,
} SSD1306ScreenRegion;

class SSD1306 : public virtual SGL {
public:
    SSD1306(uint8_t i2c_address);
    void init(void);
    void drawPixel(uint16_t x, uint16_t y, uint16_t color);

    /**
     * Buffered drawString method. This method DOES NOT automatically call
     * uploadBuffer. Suitable for drawing a lot of text at once and then
     * uploading in bulk manually.
     */ 
    void b_drawString(char *string, uint16_t x, uint16_t y, uint16_t size, uint16_t color);
    void drawString(char *string, uint16_t x, uint16_t y, uint16_t size, uint16_t color);
    void clearWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

    /**
     * Sets the region the buffer corresponds to. Note that ALL drawing is
     * relative (x0, y0). Therefore to draw a dot a (x0, y0), draw to (0, 0)
     * after calling this method.
     *
     * Note that calling this method will also zero the buffer.
     *
     * Note that y0 AND height MUST be a multiple of 8 for things to work
     * correctly.
     *
     * On success, 0 is returned. Otherwise an error code is returned.
     *
     */
    int8_t setBufferRegion(uint16_t x0, uint16_t y0, uint16_t width, uint16_t height);

    /**
     * Convenient way to set the buffer region to predefined values
     */
    int8_t setBufferRegion(SSD1306ScreenRegion region);

    /**
     * Uploads the buffer to the region previously specified by
     * setBufferRegion. This is somewhat optimised such that only
     * the buffer bounded by modified elements is uploaded.
     */
    void uploadBuffer();

    /**
     * Like uploadBuffer, but uploads everything.
     */
    void uploadBufferAll();

    /**
     * Resets the buffer to 0
     */
    void clearBuffer();

    /**
     * Fills the screen, taking care of the whole business of setting buffer
     * region and uploading it.
     */
    void fillScreen(uint16_t color);

private:
    void _sendCmd(uint8_t cmd);
    void _sendData(uint8_t* dataprt, uint16_t len);
    void _sendData(uint8_t byte);
    void _wait(void);

    uint8_t _i2c_address;
    uint8_t _buffer[SSD1306_BUF_SIZE];

    /**
     * Stores x0, y0
     */
    uint8_t _buffer_origin[2];

    /**
     * Stores width, height
     */
    uint8_t _buffer_size[2];

    /**
     * Stores the largest and smallest buffer index accessed since the last
     * uploadBuffer call
     */
    uint16_t _buffer_modified_index[2];
};

#endif
