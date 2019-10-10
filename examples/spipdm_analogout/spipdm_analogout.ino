#include <SPI.h>
#include <Adafruit_ZeroPDMSPI.h>

#define PDM_SPI            SPI2    // PDM mic SPI peripheral
#define PDM_SERCOM_HANDLER SERCOM3_0_Handler

Adafruit_ZeroPDMSPI pdmspi(&PDM_SPI);

// INTERRUPT HANDLERS ------------------------------------------------------
volatile uint16_t     voiceLastReading = 0;
void PDM_SERCOM_HANDLER(void) {
  digitalWrite(13, HIGH);
  uint16_t v = 0;
  if (pdmspi.decimateFilterWord(&v)) {
    voiceLastReading = v;
  }
  digitalWrite(13, LOW);
}

void setup() {
  while (!Serial) delay(10);
  Serial.begin(115200);
  pinMode(13, OUTPUT);
  delay(100);
  Serial.println("PDM Mic test");
  Serial.println("-------------------");
  
  // 3 MHz / 32 bits = 93,750 Hz interrupt frequency
  // 2 interrupts/sample = 46,875 Hz audio sample rate
  pdmspi.begin(46000);
  Serial.print("Final PDM frequency: "); Serial.println(pdmspi.sampleRate);

  // Set up analog output ------------------------------------------
  analogWriteResolution(12);
}


void loop() {
  int16_t val = map(voiceLastReading, 0, 65535, -2000, 2000);
  analogWrite(A0, val + 2048);
}
