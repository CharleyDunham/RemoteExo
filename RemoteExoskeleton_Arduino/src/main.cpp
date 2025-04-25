#include <Arduino.h>
#include <WebSocketBridge.h>
WebSocketBridge ws;
void setup() {
// write your initialization code here
    try {
        ws.begin();
    } catch (...) {
        Serial << "Failed to start WebSocket server." << endl;
        esp_deep_sleep_start();
    }
}

void loop() {
// write your code here
    ws.loop();
}