// Adafruit Arduino Zero / Feather M0 I2S audio library.
// Author: Tony DiCola & Limor "ladyada" Fried
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
#include "Adafruit_ZeroPDM.h"


// Define macros for debug output that optimize out when debug mode is disabled.
#ifdef DEBUG
  #define DEBUG_PRINT(...) DEBUG_PRINTER.print(__VA_ARGS__)
  #define DEBUG_PRINTLN(...) DEBUG_PRINTER.println(__VA_ARGS__)
#else
  #define DEBUG_PRINT(...)
  #define DEBUG_PRINTLN(...)
#endif


Adafruit_ZeroPDM::Adafruit_ZeroPDM(int clockpin, int datapin, gclk_generator gclk):
  _gclk(gclk), _clk(clockpin), _data(datapin)  {

}

bool Adafruit_ZeroPDM::begin(void) {
  // check the pins are valid!
  _clk_pin = _clk_mux = _data_pin = _data_mux = 0;

  // Clock pin, can only be one of 3 options
  uint32_t clockport = g_APinDescription[_clk].ulPort;
  uint32_t clockpin = g_APinDescription[_clk].ulPin;
  if ((clockport == 0) && (clockpin == 10)) {
    // PA10
    _i2sclock = I2S_CLOCK_UNIT_0;
    _clk_pin = PIN_PA10G_I2S_SCK0;
    _clk_mux = MUX_PA10G_I2S_SCK0;
  } else if ((clockport == 1) && (clockpin == 10)) {
    // PB11
    _i2sclock = I2S_CLOCK_UNIT_1;
    _clk_pin = PIN_PB11G_I2S_SCK1;
    _clk_mux = MUX_PB11G_I2S_SCK1;
  } else if ((clockport == 0) && (clockpin == 20)) {
    // PA20
    _i2sclock = I2S_CLOCK_UNIT_0;
    _clk_pin = PIN_PA20G_I2S_SCK0;
    _clk_mux = MUX_PA20G_I2S_SCK0;
  } else {
    DEBUG_PRINTLN("Clock isnt on a valid pin");
    return false;
  }

  // Data pin, can only be one of 3 options
  uint32_t datapin = g_APinDescription[_data].ulPin;
  uint32_t dataport = g_APinDescription[_data].ulPort;
  if ((dataport == 0) && (datapin == 7)) {
    // PA07
    _i2sserializer = I2S_SERIALIZER_0;
    _data_pin = PIN_PA07G_I2S_SD0;
    _data_mux = MUX_PA07G_I2S_SD0;
  } else if ((dataport == 0) && (datapin == 8)) {
    // PA08
    _i2sserializer = I2S_SERIALIZER_1;
    _data_pin = PIN_PA08G_I2S_SD1;
    _data_mux = MUX_PA08G_I2S_SD1;
  } else if ((dataport == 0) && (datapin == 19)) {
    // PA19
    _i2sserializer = I2S_SERIALIZER_0;
    _data_pin = PIN_PA19G_I2S_SD0;
    _data_mux = MUX_PA19G_I2S_SD0;
  } else {
    DEBUG_PRINTLN("Data isnt on a valid pin");
    return false;
  }

  // Initialize I2S module from the ASF.
  status_code res = i2s_init(&_i2s_instance, I2S);
  if (res != STATUS_OK) {
    DEBUG_PRINT("i2s_init failed with result: "); DEBUG_PRINTLN(res);
    return false;
  }
  return true;
}

bool Adafruit_ZeroPDM::configure(uint8_t numChannels, uint32_t sampleRateHz, uint8_t bitsPerSample) {
  // Convert bit per sample int into explicit ASF values.
  i2s_slot_size slot_size;
  i2s_data_size data_size;
  switch (bitsPerSample) {
    case 8:
      slot_size = I2S_SLOT_SIZE_8_BIT;
      data_size = I2S_DATA_SIZE_8BIT;
      break;
    case 16:
      slot_size = I2S_SLOT_SIZE_16_BIT;
      data_size = I2S_DATA_SIZE_16BIT;
      break;
    case 24:
      slot_size = I2S_SLOT_SIZE_24_BIT;
      data_size = I2S_DATA_SIZE_24BIT;
      break;
    case 32:
      slot_size = I2S_SLOT_SIZE_32_BIT;
      data_size = I2S_DATA_SIZE_32BIT;
      break;
    default:
      DEBUG_PRINTLN("Expected bits per sample to be 8, 16, 24, or 32!");
      return false;
  }

  // Disable I2S while it is being reconfigured to prevent unexpected output.
  i2s_disable(&_i2s_instance);

  // Configure the GCLK generator that will drive the I2S clocks.  This clock
  // will run at the SCK frequency by dividing the 48mhz main cpu clock.
  struct system_gclk_gen_config gclk_generator;
  // Load defaults for the clock generator.
  system_gclk_gen_get_config_defaults(&gclk_generator);
  // Set the clock generator to use the 48mhz main CPU clock and divide it down
  // to the SCK frequency.
  gclk_generator.source_clock = SYSTEM_CLOCK_SOURCE_DFLL;
  gclk_generator.division_factor = F_CPU / (sampleRateHz*numChannels*bitsPerSample);
  // Set the GCLK generator config and enable it.
  system_gclk_gen_set_config(_gclk, &gclk_generator);
  system_gclk_gen_enable(_gclk);

	// Configure I2S clock.
  struct i2s_clock_unit_config i2s_clock_instance;
	i2s_clock_unit_get_config_defaults(&i2s_clock_instance);
  // Configure source GCLK for I2S peripheral.
	i2s_clock_instance.clock.gclk_src = _gclk;
  // Disable MCK output and set SCK to MCK value.
	i2s_clock_instance.clock.mck_src = I2S_MASTER_CLOCK_SOURCE_GCLK;
	i2s_clock_instance.clock.mck_out_enable = false;
	i2s_clock_instance.clock.sck_src = I2S_SERIAL_CLOCK_SOURCE_MCKDIV;
	i2s_clock_instance.clock.sck_div = 1;
  // Configure number of channels and slot size (based on bits per sample).
	i2s_clock_instance.frame.number_slots = numChannels;
	i2s_clock_instance.frame.slot_size = slot_size;
  // Configure 1-bit delay in each frame (I2S default).
  i2s_clock_instance.frame.data_delay = I2S_DATA_DELAY_1;
  // Configure FS generation from SCK clock.
	i2s_clock_instance.frame.frame_sync.source = I2S_FRAME_SYNC_SOURCE_SCKDIV;
  // Configure FS change on full slot change (I2S default).
	i2s_clock_instance.frame.frame_sync.width = I2S_FRAME_SYNC_WIDTH_SLOT;
	// Disable MCK pin output and FS pin (unneeded)
	i2s_clock_instance.mck_pin.enable = false;
	i2s_clock_instance.fs_pin.enable = false;
  // Enable SCK pin output
	i2s_clock_instance.sck_pin.enable = true;
	i2s_clock_instance.sck_pin.gpio = _clk_pin;
	i2s_clock_instance.sck_pin.mux = _clk_mux;
  // Set clock configuration.
	status_code res = i2s_clock_unit_set_config(&_i2s_instance, _i2sclock, &i2s_clock_instance);
  if (res != STATUS_OK) {
    DEBUG_PRINT("i2s_clock_unit_set_config failed with result: "); DEBUG_PRINTLN(res);
    return false;
  }

  // Configure I2S serializer.
	struct i2s_serializer_config i2s_serializer_instance;
	i2s_serializer_get_config_defaults(&i2s_serializer_instance);
  // Configure clock unit to use with serializer, and set serializer as an output.
	i2s_serializer_instance.clock_unit = _i2sclock;
	i2s_serializer_instance.mode = I2S_SERIALIZER_RECEIVE;
  // Configure serializer data size.
	i2s_serializer_instance.data_size = data_size;
  // Enable SD pin.  See Adafruit_ZeroI2S.h for default pin value.
	i2s_serializer_instance.data_pin.enable = true;
	i2s_serializer_instance.data_pin.gpio = _data_pin;
	i2s_serializer_instance.data_pin.mux = _data_mux;
	res = i2s_serializer_set_config(&_i2s_instance, _i2sserializer, &i2s_serializer_instance);
  if (res != STATUS_OK) {
    DEBUG_PRINT("i2s_serializer_set_config failed with result: "); DEBUG_PRINTLN(res);
    return false;
  }

  // Enable everything configured above.
  i2s_enable(&_i2s_instance);
  i2s_clock_unit_enable(&_i2s_instance, _i2sclock);
  i2s_serializer_enable(&_i2s_instance, _i2sserializer);

  return true;
}

uint32_t Adafruit_ZeroPDM::read(void) {
  // Read the sample from the I2S data register.
  // This will wait for the I2S hardware to be ready to send the byte.
  return i2s_serializer_read_wait(&_i2s_instance, _i2sserializer);
}


bool Adafruit_ZeroPDM::read(uint32_t *buffer, int bufsiz) {
  // Read the sample from the I2S data register.
  // This will wait for the I2S hardware to be ready to send the byte.
  // Write the sample byte to the I2S data register.
  // This will wait for the I2S hardware to be ready to receive the byte.
  status_code stat = i2s_serializer_read_buffer_wait(&_i2s_instance, _i2sserializer, buffer, bufsiz);

  return (stat == STATUS_OK); // anything other than OK is a problem
}
