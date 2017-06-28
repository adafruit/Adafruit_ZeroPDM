#include "Adafruit_ASFcore.h"
#include "Adafruit_ZeroPDM.h"
#include <Adafruit_NeoPixel.h>
#include <SPI.h>

#define SAMPLERATE_HZ 44100
#define DECIMATION    64

// Create PDM receiver object, with Clock and Data pins used (not all pins available)
Adafruit_ZeroPDM pdm = Adafruit_ZeroPDM(1, 4);  // Metro M0 or Arduino zero
//Adafruit_ZeroPDM pdm = Adafruit_ZeroPDM(34, 35);  // CPlay express

#define SERIALPORT Serial

// a windowed sinc filter for 44 khz, 64 samples
uint16_t sincfilter[DECIMATION] = {0, 2, 9, 21, 39, 63, 94, 132, 179, 236, 302, 379, 467, 565, 674, 792, 920, 1055, 1196, 1341, 1487, 1633, 1776, 1913, 2042, 2159, 2263, 2352, 2422, 2474, 2506, 2516, 2506, 2474, 2422, 2352, 2263, 2159, 2042, 1913, 1776, 1633, 1487, 1341, 1196, 1055, 920, 792, 674, 565, 467, 379, 302, 236, 179, 132, 94, 63, 39, 21, 9, 2, 0, 0};

void setup() {
  // Configure serial port.
  SERIALPORT.begin(9600);
  SERIALPORT.println("SAMD PDM Demo");
  pinMode(13, OUTPUT);

  // Initialize the PDM/I2S receiver
  if (!pdm.begin()) {
    SERIALPORT.println("Failed to initialize I2S/PDM!");
    while (1);
  }
  SERIALPORT.println("PDM initialized");

  // Configure PDM receiver, sample rate
  if (!pdm.configure(SAMPLERATE_HZ * DECIMATION / 32)) {
    SERIALPORT.println("Failed to configure PDM");
    while (1);
  }
  SERIALPORT.println("PDM configured");

  // use analog output A0 @ full rez
  analogWriteResolution(10);
}


uint16_t readings[DECIMATION / 16];  // 4 x 16 bit words = 64 samples to match our filter!
void loop() {
  for (uint8_t i=0; i < (DECIMATION/16) ; i++) {
     readings[i] = pdm.read() & 0xFFFF;
  }

  uint16_t runningsum = 0;
  for (uint8_t samplenum = 0; samplenum < (DECIMATION/16); samplenum++) {
    uint16_t sample = readings[samplenum];
    for (int8_t i=0; i<16; i++) {
      //SERIALPORT.print(" ["); SERIALPORT.print( filter[samplenum * 16 + i]); SERIALPORT.print("] ");

      // start at the MSB which is the 'first' bit to come down the line, chronologically
      if (sample & 0x8000) {
        runningsum += sincfilter[samplenum * 16 + i];
        //SERIALPORT.print(" + "); SERIALPORT.print(filter[samplenum * 16 + i]);
      }
      sample <<= 1;
    }
  }

  // since we wait for the samples from I2S peripheral, we dont need to delay, we will 'naturally'
  // wait the right amount of time between analog writes
  analogWrite(A0, runningsum >> 6); // convert 16 bit -> 10 bit

  // note that we cannot print the the serial port fast enough to keep up!
}
