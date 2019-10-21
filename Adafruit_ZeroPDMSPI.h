// Adafruit SAMD51 PDM via SPI mic library.
// Author: PaintYourDragon & Limor "Ladyada" Fried
//
// The MIT License (MIT)
//
// Copyright (c) 2019 Adafruit Industries
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
#ifndef ADAFRUIT_ZEROPDMSPI_H
#define ADAFRUIT_ZEROPDMSPI_H

#if defined(__SAMD51__)

#include <Arduino.h>
#include <SPI.h>
#include "wiring_private.h"

class Adafruit_ZeroPDMSPI {
public:
  // Create a new instance of an PDM audio transmitter over SPI
  Adafruit_ZeroPDMSPI(SPIClass *theSPI); 

  // Initialize the SPI audio receiver.
  bool begin(uint32_t freq);

  bool decimateFilterWord(uint16_t *value, bool removeDC=true); 

  void setMicGain(float g=1.0);

  float sampleRate;
private:
  SPIClass *_spi = NULL;
  Sercom *_sercom = NULL;
  IRQn_Type _irq;
  volatile uint32_t *_dataReg;

  uint16_t       dcCounter        = 0;     // Rolls over every DC_PERIOD samples
  uint32_t       dcSum            = 0;     // Accumulates DC_PERIOD samples
  uint16_t       dcOffsetPrior    = 32768; // DC offset interpolates linearly
  uint16_t       dcOffsetNext     = 32768; // between these two values

  uint16_t       micGain          = 256;   // 1:1
};

#endif //defined(__SAMD51__)

#endif
