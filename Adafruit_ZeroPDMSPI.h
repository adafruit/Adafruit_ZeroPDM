// Adafruit Arduino Zero / Feather M0 PDM mic library.
// Author: Tony DiCola & Limor "Ladyada" Fried
//
// The MIT License (MIT)
//
// Copyright (c) 2016 Adafruit Industries
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

#include <Arduino.h>
#include <SPI.h>
#include "wiring_private.h"

// Uncomment to enable debug message output.
#define DEBUG

// Define where debug output is printed (the native USB port on the Zero).
#define DEBUG_PRINTER Serial


class Adafruit_ZeroPDMSPI {
public:
  // Create a new instance of an PDM audio transmitter over SPI
  Adafruit_ZeroPDMSPI(SPIClass *theSPI); 

  // Initialize the SPI audio receiver.
  bool begin();
  void end();

  // Configure the transmitter with the sample rate (in hertz
  bool configure(uint32_t sampleRateHz, boolean stereo);

  // Read a single sample from the SPI subsystem.  Will wait until the SPI
  // hardware is ready to receive the sample.
  uint32_t read(void);

  bool read(uint32_t *buffer, int bufsiz);

  float sampleRate;
private:
  SPIClass *_spi = NULL;
  Sercom *_sercom = NULL;
  IRQn_Type _irq;
};

#endif
