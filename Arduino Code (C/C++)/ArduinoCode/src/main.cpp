#include <Arduino.h>
#include <FS.h>

#include <SPIFFS.h>
#include "SerialStream.h"
#include "WebSocketBridge.h"
WebSocketBridge bridge;
bool bridgeFail = false;
void setup() {
    // 1) USB‑CDC
    Serial.begin(115200);
    esp_rom_delay_us(3000000);              // wait for the CDC port to come online
    try {
        bridge.begin();
        Serial << "WebSocket Bridge started" << endl;
    } catch (const std::exception& e) {
        Serial << "WebSocket Bridge failed to start: " << e.what() << endl;
        bridgeFail = true;
    }
    // 2) LED
    pinMode(LED_BUILTIN, OUTPUT);

}

void loop() {
    // now the LED will actually blink
    if (!bridgeFail) {
        bridge.loop();
    } else {
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        delay(100);
    }
}
