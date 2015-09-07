#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>

// defined in main.cpp
extern "C" void do_main();

// Debug function, to dump a frame from inside main.cpp. Marked extern
// "C" so you can also use it inside the library .c files if needed
extern "C" void printframe(uint8_t *buf, size_t len);

extern "C" void printframe(uint8_t *buf, size_t len) {
  return;
  for (size_t i = 0; i < len; ++i) {
    if (buf[i] < 0x10)
      Serial.write('0');
    Serial.print(buf[i], len);
    Serial.write(' ');
  }
}

// Pin mapping
lmic_pinmap pins = {
  .nss = SS,
  .rxtx = 8, // Not connected
  .rst = A7, // Not connected
  .dio = {4, 5, 7},
};

void setup() {
  Serial.begin(115200);
  Serial.println("Starting");
  pinMode(LED_BUILTIN, OUTPUT);

  #ifdef VCC_ENABLE
  // For Pinoccio Scout boards
  pinMode(VCC_ENABLE, OUTPUT);
  digitalWrite(VCC_ENABLE, HIGH);
  delay(1000);
  #endif

  do_main();
}

void loop() { }

