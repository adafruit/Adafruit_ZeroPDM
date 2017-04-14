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
#ifndef ADAFRUIT_ZEROPDM_H
#define ADAFRUIT_ZEROPDM_H

#include <Arduino.h>
#include <sam.h>
#include <i2s.h>

/*
//#define I2S_SDRX_PIN PIN_PA07G_I2S_SD0
//#define I2S_SDRX_MUX MUX_PA07G_I2S_SD0
//#define I2S_SDRX_SERIALIZER I2S_SERIALIZER_0

#define I2S_SDRX_PIN PIN_PA08G_I2S_SD1
#define I2S_SDRX_MUX MUX_PA08G_I2S_SD1
#define I2S_SDRX_SERIALIZER I2S_SERIALIZER_1

#define I2S_CLKRX_PIN PIN_PA10G_I2S_SCK0
#define I2S_CLKRX_MUX MUX_PA10G_I2S_SCK0
#define I2S_SDRX_CLOCK I2S_CLOCK_UNIT_0

//#define I2S_CLKRX_PIN PIN_PB11G_I2S_SCK1
//#define I2S_CLKRX_MUX MUX_PB11G_I2S_SCK1
//#define I2S_SDRX_CLOCK I2S_CLOCK_UNIT_1
*/

#define I2S_SDRX_SERIALIZERMODE I2S_SERIALIZER_RECEIVE

// Uncomment to enable debug message output.
#define DEBUG

// Define where debug output is printed (the native USB port on the Zero).
#define DEBUG_PRINTER Serial


class Adafruit_ZeroPDM {
public:
  // Create a new instance of an I2S audio transmitter.
  // Can specify the pins to use and the generic clock ID to use for driving the I2S
  // hardware (default is GCLK 3).
  Adafruit_ZeroPDM(int clockpin, int datapin, gclk_generator gclk = GCLK_GENERATOR_3); 

  // Initialize the I2S audio  receiver.
  bool begin();

  // Configure the transmitter with the specified number of channels, sample rate
  // (in hertz), and number of bits per sample (one of 8, 16, 24, 32).
  bool configure(uint8_t numChannels, uint32_t sampleRateHz, uint8_t bitsPerSample);

  // Read a single sample from the I2S subsystem.  Will wait until the I2S
  // hardware is ready to receive the sample.
  uint32_t read(void);
  bool read(uint32_t *buffer, int bufsiz);

private:
  int _clk, _data;
  uint32_t _clk_pin, _clk_mux, _data_pin, _data_mux;
  i2s_serializer _i2sserializer;
  i2s_clock_unit _i2sclock;
  gclk_generator _gclk;
  struct i2s_module _i2s_instance;
};


#endif
