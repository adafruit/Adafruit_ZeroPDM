#include "Adafruit_ZeroPDMSPI.h"

volatile uint32_t *dataReg;

// PDM mic allows 1.0 to 3.25 MHz max clock (2.4 typical).
// SPI native max is is 24 MHz, so available speeds are 12, 6, 3 MHz.
#define SPI_BITRATE 3000000
static SPISettings settings(SPI_BITRATE, LSBFIRST, SPI_MODE0);

static Sercom * const sercomList[] = {
  SERCOM0, SERCOM1, SERCOM2, SERCOM3,
#if defined(SERCOM4)
  SERCOM4,
#endif
#if defined(SERCOM5)
  SERCOM5,
#endif
#if defined(SERCOM6)
  SERCOM6,
#endif
#if defined(SERCOM7)
  SERCOM7,
#endif
};

static IRQn_Type const sercomIRQList[] {
  SERCOM0_0_IRQn, SERCOM1_0_IRQn, SERCOM2_0_IRQn, SERCOM3_0_IRQn,
#if defined(SERCOM4)
  SERCOM4_0_IRQn,
#endif
#if defined(SERCOM5)
  SERCOM5_0_IRQn,
#endif
#if defined(SERCOM6)
  SERCOM6_0_IRQn,
#endif
#if defined(SERCOM7)
  SERCOM7_0_IRQn,
#endif
};

Adafruit_ZeroPDMSPI::Adafruit_ZeroPDMSPI(SPIClass *theSPI) {
  _spi = theSPI;
}

bool Adafruit_ZeroPDMSPI::begin() {

  _spi->begin();
  _spi->beginTransaction(settings); // this SPI transaction is left open
  _sercom  = sercomList[_spi->getSercomIndex()];
  _irq = sercomIRQList[_spi->getSercomIndex()];
  dataReg = _spi->getDataRegister();

  // Enabling 32-bit SPI must be done AFTER SPI.begin() which
  // resets registers. But SPI.CTRLC (where 32-bit mode is set) is
  // enable-protected, so peripheral must be disabled temporarily...
  _sercom->SPI.CTRLA.bit.ENABLE  = 0;      // Disable SPI
  while(_sercom->SPI.SYNCBUSY.bit.ENABLE); // Wait for disable
  _sercom->SPI.CTRLC.bit.DATA32B = 1;      // Enable 32-bit mode
  _sercom->SPI.CTRLA.bit.ENABLE  = 1;      // Re-enable SPI
  while(_sercom->SPI.SYNCBUSY.bit.ENABLE); // Wait for enable
  // 4-byte word length is implicit in 32-bit mode,
  // no need to set up LENGTH register.

  _sercom->SPI.INTENSET.bit.DRE  = 1;      // Data-register-empty interrupt
  NVIC_DisableIRQ(_irq);
  NVIC_ClearPendingIRQ(_irq);
  NVIC_SetPriority(_irq, 0);   // Top priority
  NVIC_EnableIRQ(_irq);

  _sercom->SPI.DATA.bit.DATA     = 0;      // Kick off SPI free-run

  // 3 MHz / 32 bits = 93,750 Hz interrupt frequency
  // 2 interrupts/sample = 46,875 Hz audio sample rate
  sampleRate = (float)SPI_BITRATE / 64.0;
  // sampleRate is float in case factors change to make it not divide evenly.
  // It DOES NOT CHANGE over time, only playbackRate does.

  return true; // Success
}
