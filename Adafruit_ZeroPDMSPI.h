/**************************************************************************/
/*!
 * @file Adafruit_ZeroPDMSPI.h
 *
 *  Adafruit Arduino Zero / Feather M0 PDM mic library.
 *
 *  Adafruit invests time and resources providing this open source code,
 *  please support Adafruit and open-source hardware by purchasing
 *  products from Adafruit!
 *
 *  Written by Tony DiCola & Limor "ladyada" Fried for Adafruit Industries.
 *
 * MIT license, check license.txt for more information. All text above must be
 * included in any redistribution
 *
 */
/**************************************************************************/
#ifndef ADAFRUIT_ZEROPDMSPI_H
#define ADAFRUIT_ZEROPDMSPI_H
/// @cond SAMD51
#if defined(__SAMD51__)

/// @endcond
#include "wiring_private.h"
#include <Arduino.h>
#include <SPI.h>

class Adafruit_ZeroPDMSPI {
public:
  // Create a new instance of an PDM audio transmitter over SPI
  Adafruit_ZeroPDMSPI(SPIClass *theSPI);

  // Initialize the SPI audio receiver.
  bool begin(uint32_t freq);

  bool decimateFilterWord(uint16_t *value, bool removeDC = true);

  void setMicGain(float g = 1.0);

  float sampleRate;

private:
  SPIClass *_spi = NULL;
  Sercom *_sercom = NULL;
  IRQn_Type _irq;
  volatile uint32_t *_dataReg;

  uint16_t dcCounter = 0;         // Rolls over every DC_PERIOD samples
  uint32_t dcSum = 0;             // Accumulates DC_PERIOD samples
  uint16_t dcOffsetPrior = 32768; // DC offset interpolates linearly
  uint16_t dcOffsetNext = 32768;  // between these two values

  uint16_t micGain = 256; // 1:1
};
/// @cond SAMD51
#endif // defined(__SAMD51__)
/// @endcond
#endif
