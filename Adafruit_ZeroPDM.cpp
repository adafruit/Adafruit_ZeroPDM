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

void Adafruit_ZeroPDM::end(void) {
  //replace "i2s_disable(&_i2s_instance);" with:
  
  while (_i2s_instance.hw->SYNCBUSY.reg & I2S_SYNCBUSY_ENABLE); // Sync wait
  _i2s_instance.hw->CTRLA.reg &= ~I2S_SYNCBUSY_ENABLE;
}

bool Adafruit_ZeroPDM::configure(uint32_t sampleRateHz, boolean stereo) {
  // Convert bit per sample int into explicit ASF values.

  // Disable I2S while it is being reconfigured to prevent unexpected output.
  //replace "i2s_disable(&_i2s_instance);" with:
  end();

  // Configure the GCLK generator that will drive the I2S clocks.  This clock
  // will run at the SCK frequency by dividing the 48mhz main cpu clock.
  struct system_gclk_gen_config gclk_generator;
  // Load defaults for the clock generator.
  system_gclk_gen_get_config_defaults(&gclk_generator);
  // Set the clock generator to use the 48mhz main CPU clock and divide it down
  // to the SCK frequency.
  gclk_generator.source_clock = SYSTEM_CLOCK_SOURCE_DFLL;
  gclk_generator.division_factor = F_CPU / (sampleRateHz*16); // 16 clocks for 16 stereo bits

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
  if (stereo) {
    i2s_clock_instance.frame.number_slots = 2; // must be stereo for PDM2
    i2s_clock_instance.frame.slot_size = I2S_SLOT_SIZE_16_BIT; // must be 16 bits (32 bit word containing stereo data)
  } else {
    i2s_clock_instance.frame.number_slots = 2;
    i2s_clock_instance.frame.slot_size = I2S_SLOT_SIZE_32_BIT;
  }

  // Configure 1-bit delay in each frame
  i2s_clock_instance.frame.data_delay = I2S_DATA_DELAY_0;
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
  // Replace "i2s_serializer_get_config_defaults(&i2s_serializer_instance);" with:

  i2s_serializer_instance.loop_back = false;
  i2s_serializer_instance.mono_mode = false;
  i2s_serializer_instance.disable_data_slot[0] = false;
  i2s_serializer_instance.disable_data_slot[1] = false;
  i2s_serializer_instance.disable_data_slot[2] = false;
  i2s_serializer_instance.disable_data_slot[3] = false;
  i2s_serializer_instance.disable_data_slot[4] = false;
  i2s_serializer_instance.disable_data_slot[5] = false;
  i2s_serializer_instance.disable_data_slot[6] = false;
  i2s_serializer_instance.disable_data_slot[7] = false;
  i2s_serializer_instance.transfer_lsb_first = false;
  i2s_serializer_instance.data_adjust_left_in_word = false;
  i2s_serializer_instance.data_adjust_left_in_slot = true;
  i2s_serializer_instance.data_size = I2S_DATA_SIZE_16BIT;
  i2s_serializer_instance.bit_padding = I2S_BIT_PADDING_0;
  i2s_serializer_instance.data_padding = I2S_DATA_PADDING_0;
  i2s_serializer_instance.dma_usage = I2S_DMA_USE_SINGLE_CHANNEL_FOR_ALL;
  i2s_serializer_instance.clock_unit = I2S_CLOCK_UNIT_0;
  i2s_serializer_instance.line_default_state = I2S_LINE_DEFAULT_0;
  i2s_serializer_instance.mode = I2S_SERIALIZER_TRANSMIT;
  i2s_serializer_instance.data_pin.enable = false;
  i2s_serializer_instance.data_pin.gpio = 0;
  i2s_serializer_instance.data_pin.mux = 0;
  

  // Configure clock unit to use with serializer, and set serializer as an output.
  i2s_serializer_instance.clock_unit = _i2sclock;
  if (stereo) {
    i2s_serializer_instance.mode = I2S_SERIALIZER_PDM2; //Serializer is used to receive PDM data on each clock edge
  } else {
    i2s_serializer_instance.mode = I2S_SERIALIZER_RECEIVE; // act like I2S
  }
  // Configure serializer data size.
  i2s_serializer_instance.data_size = I2S_DATA_SIZE_32BIT; // anything other than 32 bits is ridiculous to manage, force this to be 32
  // Enable SD pin.  See Adafruit_ZeroI2S.h for default pin value.
  i2s_serializer_instance.data_pin.enable = true;
  i2s_serializer_instance.data_pin.gpio = _data_pin;
  i2s_serializer_instance.data_pin.mux = _data_mux;
  

  /*
  res = i2s_serializer_set_config(&_i2s_instance, _i2sserializer, &i2s_serializer_instance);
  if (res != STATUS_OK) {
    DEBUG_PRINT("i2s_serializer_set_config failed with result: "); DEBUG_PRINTLN(res);
    return false;
  }
  */


  /* Status check */
  /* Busy ? */
  while (_i2s_instance.hw->SYNCBUSY.reg & ((I2S_SYNCBUSY_SEREN0 | I2S_SYNCBUSY_DATA0) << _i2sserializer)) {
    //return STATUS_BUSY;
    return false;
  }

  /* Already enabled ? */
  if (_i2s_instance.hw->CTRLA.reg & (I2S_CTRLA_CKEN0 << _i2sserializer)) {
    // return STATUS_ERR_DENIED;
    return false;
  }

  /* Initialize Serializer */
  uint32_t serctrl =
    (i2s_serializer_instance.loop_back ? I2S_SERCTRL_RXLOOP : 0) |
    (i2s_serializer_instance.dma_usage ? I2S_SERCTRL_DMA : 0) |
    (i2s_serializer_instance.mono_mode ? I2S_SERCTRL_MONO : 0) |
    (i2s_serializer_instance.disable_data_slot[7] ? I2S_SERCTRL_SLOTDIS7 : 0) |
    (i2s_serializer_instance.disable_data_slot[6] ? I2S_SERCTRL_SLOTDIS6 : 0) |
    (i2s_serializer_instance.disable_data_slot[5] ? I2S_SERCTRL_SLOTDIS5 : 0) |
    (i2s_serializer_instance.disable_data_slot[4] ? I2S_SERCTRL_SLOTDIS4 : 0) |
    (i2s_serializer_instance.disable_data_slot[3] ? I2S_SERCTRL_SLOTDIS3 : 0) |
    (i2s_serializer_instance.disable_data_slot[2] ? I2S_SERCTRL_SLOTDIS2 : 0) |
    (i2s_serializer_instance.disable_data_slot[1] ? I2S_SERCTRL_SLOTDIS1 : 0) |
    (i2s_serializer_instance.disable_data_slot[0] ? I2S_SERCTRL_SLOTDIS0 : 0) |
    (i2s_serializer_instance.transfer_lsb_first ? I2S_SERCTRL_BITREV : 0) |
    (i2s_serializer_instance.data_adjust_left_in_word ? I2S_SERCTRL_WORDADJ : 0) |
    (i2s_serializer_instance.data_adjust_left_in_slot ? I2S_SERCTRL_SLOTADJ : 0) |
    (i2s_serializer_instance.data_padding ? I2S_SERCTRL_TXSAME : 0);

  if (i2s_serializer_instance.clock_unit < I2S_CLOCK_UNIT_N) {
    serctrl |= (i2s_serializer_instance.clock_unit ? I2S_SERCTRL_CLKSEL : 0);
  } else {
    //return STATUS_ERR_INVALID_ARG;
    return false;
  }
  
  serctrl |=
    I2S_SERCTRL_SERMODE(i2s_serializer_instance.mode) |
    I2S_SERCTRL_TXDEFAULT(i2s_serializer_instance.line_default_state) |
    I2S_SERCTRL_DATASIZE(i2s_serializer_instance.data_size) |
    I2S_SERCTRL_EXTEND(i2s_serializer_instance.bit_padding);
  
  /* Write Serializer configuration */
  _i2s_instance.hw->SERCTRL[_i2sserializer].reg = serctrl;

  /* Initialize pins */
  struct system_pinmux_config pin_config;
  system_pinmux_get_config_defaults(&pin_config);
  if (i2s_serializer_instance.data_pin.enable) {
    pin_config.mux_position = i2s_serializer_instance.data_pin.mux;
    system_pinmux_pin_set_config(i2s_serializer_instance.data_pin.gpio, &pin_config);
  }

  /* Save configure */
  _i2s_instance.serializer[_i2sserializer].mode = i2s_serializer_instance.mode;
  _i2s_instance.serializer[_i2sserializer].data_size = i2s_serializer_instance.data_size;

  /* Enable everything configured above. */

  // Replace "i2s_enable(&_i2s_instance);" with:
  while (_i2s_instance.hw->SYNCBUSY.reg & I2S_SYNCBUSY_ENABLE);  // Sync wait
  _i2s_instance.hw->CTRLA.reg |= I2S_SYNCBUSY_ENABLE;

  // Replace "i2s_clock_unit_enable(&_i2s_instance, _i2sclock);" with:
  uint32_t cken_bit = I2S_CTRLA_CKEN0 << _i2sclock;
  while (_i2s_instance.hw->SYNCBUSY.reg & cken_bit); // Sync wait
  _i2s_instance.hw->CTRLA.reg |= cken_bit;

  // Replace "i2s_serializer_enable(&_i2s_instance, _i2sserializer);" with:
  uint32_t seren_bit = I2S_CTRLA_SEREN0 << _i2sserializer;
  while (_i2s_instance.hw->SYNCBUSY.reg & seren_bit); // Sync wait
  _i2s_instance.hw->CTRLA.reg |= seren_bit;

  return true;
}

uint32_t Adafruit_ZeroPDM::read(void) {
  // Read the sample from the I2S data register.
  // This will wait for the I2S hardware to be ready to send the byte.
  //return i2s_serializer_read_wait(&_i2s_instance, _i2sserializer);

  // replace i2s_serializer_read_wait with deASF'd code:
  uint32_t sync_bit, ready_bit;
  uint32_t data;
  ready_bit = I2S_INTFLAG_RXRDY0 << _i2sserializer;
  while (!(_i2s_instance.hw->INTFLAG.reg & ready_bit)) {
    /* Wait until ready to transmit */
  }
  sync_bit = I2S_SYNCBUSY_DATA0 << _i2sserializer;
  while (_i2s_instance.hw->SYNCBUSY.reg & sync_bit) {
    /* Wait sync */
  }
  /* Read data */
  data = _i2s_instance.hw->DATA[_i2sserializer].reg;
  _i2s_instance.hw->INTFLAG.reg = ready_bit;
  return data;
}


bool Adafruit_ZeroPDM::read(uint32_t *buffer, int bufsiz) {
  // Read the sample from the I2S data register.
  // This will wait for the I2S hardware to be ready to send the byte.
  // Write the sample byte to the I2S data register.
  // This will wait for the I2S hardware to be ready to receive the byte.
  status_code stat = i2s_serializer_read_buffer_wait(&_i2s_instance, _i2sserializer, buffer, bufsiz);

  return (stat == STATUS_OK); // anything other than OK is a problem
}
