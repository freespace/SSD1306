#include <string.h>

#include "SSD1306.h"
#include <Wire.h>

SSD1306::SSD1306(uint8_t i2c_address) : SGL(SSD1306_LCDWIDTH, SSD1306_LCDHEIGHT)
{
  _i2c_address = i2c_address;
};

void SSD1306::_sendCmd(uint8_t c) {
    Wire.beginTransmission(_i2c_address);

    // this a command, so D/C  = 0
    Wire.write(0x00);
    Wire.write(c);
    Wire.endTransmission();
}

void SSD1306::_sendData(uint8_t byte) {
  Wire.beginTransmission(_i2c_address);

  // this is data, so D/C=1
  Wire.write(0x40);
  Wire.write(byte);

  Wire.endTransmission();
}

void SSD1306::_sendData(uint8_t *dataptr, uint16_t len) {
  // This is needed b/c the Wire library doesn't send data until
  // endTransmission is called. In the mean time, anything you try to send is
  // buffered, BUT the buffer is only 32 bytes long. So if we try to send too
  // many bytes at once without calling endTransmission, the extra bytes are
  // discarded!
  //
  // For some reason setting the burst size higher than 16 causes display
  // artifacts. No idea why. This trick comes from Adafruit:
  //
  //   https://github.com/adafruit/Adafruit_SSD1306/blob/master/Adafruit_SSD1306.cpp#L518
  //
  // Assuming this is some interaction between the Wire library and SSD1306.
  //
  // TODO: optimise this better at some point
#define I2C_BURST_SIZE       (16)
  for(uint16_t idx = 0; idx < len; idx += I2C_BURST_SIZE) {
    Wire.beginTransmission(_i2c_address);

    // this is data, so D/C=1
    Wire.write(0x40);

    for (uint8_t jdx = 0; jdx < I2C_BURST_SIZE; ++jdx) {
      Wire.write(dataptr[jdx+idx]);
    }

    Wire.endTransmission();
  }

#undef I2C_BURST_SIZE
}

void SSD1306::_wait(void) {
  delay(10);
}

void SSD1306::init(void) {
  Wire.begin();

  _sendCmd(SSD1306_DISPLAYOFF);                    // 0xAE

  _sendCmd(SSD1306_SETDISPLAYCLOCKDIV);            // 0xD5
  _sendCmd(0x80);                                  // the suggested ratio 0x80

  _sendCmd(SSD1306_SETMULTIPLEX);                  // 0xA8
  _sendCmd(0x3F);

  _sendCmd(SSD1306_SETDISPLAYOFFSET);              // 0xD3
  _sendCmd(0x0);                                   // no offset

  _sendCmd(SSD1306_SETSTARTLINE | 0x0);            // line #0

  _sendCmd(SSD1306_CHARGEPUMP);                    // 0x8D
  _sendCmd(0x14);                                  // use charge pump

  _sendCmd(SSD1306_MEMORYMODE);                    // 0x20
  _sendCmd(0x01);                                  // 0 - horizontal addressing
                                                   // 1 - vertical addressing
                                                   // 3 - page addressing
                                                   // https://github.com/guyc/py-gaugette/blob/master/gaugette/ssd1306.py

  _sendCmd(SSD1306_SEGREMAP | 0x1);

  _sendCmd(SSD1306_COMSCANDEC);

  _sendCmd(SSD1306_SETCOMPINS);                    // 0xDA
  _sendCmd(0x12);

  _sendCmd(SSD1306_SETCONTRAST);                   // 0x81
  _sendCmd(0xCF);

  _sendCmd(SSD1306_SETPRECHARGE);                  // 0xd9
  _sendCmd(0xF1);

  _sendCmd(SSD1306_SETVCOMDETECT);                 // 0xDB
  _sendCmd(0x40);

  // the following is from
  // https://code.google.com/p/u8glib/source/browse/csrc/u8g_dev_ssd1306_128x64.c
  _sendCmd(SSD1306_DEACTIVATE_SCROLL);
  _sendCmd(SSD1306_DISPLAYALLON_RESUME);           // 0xA4
  _sendCmd(SSD1306_NORMALDISPLAY);

  _sendCmd(SSD1306_DISPLAYON);

  // clear the display
  fillScreen(COLOR_BLACK);
}

void SSD1306::drawPixel(uint16_t x, uint16_t y, uint16_t color) {
  uint16_t idx = x * _buffer_size[1]/8 + y/8;
  if (color) bitSet(_buffer[idx], y%8);
  else bitClear(_buffer[idx], y%8);
}

void SSD1306::clearBuffer() {
  memset(_buffer, 0, SSD1306_BUF_SIZE);
}

int8_t SSD1306::setBufferRegion(uint16_t x0, uint16_t y0, uint16_t width, uint16_t height) {
  if (y0%8 != 0 || height%8 != 0) return SSD1306_ERROR_NOT_MULTIPLE_OF_8;
  if (width * height/8 > SSD1306_BUF_SIZE) return SSD1306_ERROR_REGION_TOOLARGE;

  _buffer_origin[0] = x0;
  _buffer_origin[1] = y0;

  _buffer_size[0] = width;
  _buffer_size[1] = height;

  memset(_buffer, 0, SSD1306_BUF_SIZE);
}

void SSD1306::uploadBuffer() {
  _sendCmd(SSD1306_COLUMNADDR);
  _sendCmd(_buffer_origin[0]);
  _sendCmd(_buffer_origin[0]+_buffer_size[0]-1);

  _sendCmd(SSD1306_PAGEADDR);
  _sendCmd(_buffer_origin[1]/8);
  _sendCmd(_buffer_origin[1]/8+_buffer_size[1]/8-1);
  _sendData(_buffer, _buffer_size[0] * _buffer_size[1]/8);
}

void SSD1306::clearWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    fillRectangle(x0, y0, x1-x0, y1-y0, 0x0);
}

void SSD1306::fillScreen(uint16_t color) {
  // clear the display
  setBufferRegion(0, 0, 128, 32);
  if (color) memset(_buffer, 0xFF, SSD1306_BUF_SIZE);
  uploadBuffer();
  setBufferRegion(0, 32, 128, 32);
  if (color) memset(_buffer, 0xFF, SSD1306_BUF_SIZE);
  uploadBuffer();
}
