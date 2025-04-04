#include "Command.h"
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <AsyncWebSocket.h>
#include <WiFi.h>
#include "Servo.h"
#include <queue>
#define WIFI_AP_SSID "RemoteExoskeleton"
#define WIFI_AP_PASS "remoteExoskeleton"
#include "html_page.h"
bool printUpdates = true;
#define PRINT_HEADER(header) do { Serial.print("-----------------"); Serial.print(header); PRINT_UPDATELN("-----------------"); } while (0)

AsyncWebSocket ws("/ws");
AsyncWebServer server(80);
ESPServo servo;
CommandProcessor processor(servo);
String currentCommand = "";

std::queue<CommandProcessor::CommandInput> receivedCommands;

void serialCallback(void *handleArgs, esp_event_base_t eventBase, int32_t eventID, void *eventData) {
    if (eventID == ARDUINO_USB_CDC_RX_EVENT) {
        currentCommand.clear();
        while (Serial.available()) {
            int data = Serial.read();
            if (data >= 0) {
                char c = static_cast<char>(data);
                if (c == '\n') {
                    currentCommand.trim();
                    receivedCommands.push({currentCommand, nullptr});
                } else {
                    currentCommand += c;
                }
            }
        }
    }
}
void websocketCallback(AsyncWebSocket *s, AsyncWebSocketClient *client, 
                       AwsEventType event, void *arg, const uint8_t *payload, size_t len) {
    switch (event) {
        case WS_EVT_DATA: {
            String msgStr((const char*)payload, len);
            receivedCommands.push({msgStr, client});
        } break;
        case WS_EVT_CONNECT:
            PRINTF_UPDATELN("Client %u connected.", client->id());
            break;
        case WS_EVT_DISCONNECT:
            PRINTF_UPDATELN("Client %u disconnected.", client->id());
            ws.cleanupClients();
            break;
        default:
            break;
    }
}
void setup() {
    Serial.onEvent(ARDUINO_USB_CDC_ANY_EVENT, &serialCallback);
    Serial.begin(115200);
    esp_rom_delay_us(1000000);
    PRINT_UPDATELN("------------------------------");
    PRINT_UPDATELN("------------------------------");
    PRINT_UPDATELN("------------------------------");
    PRINT_UPDATELN("Starting soft AP...");
    //WiFi.begin();
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS);
    PRINT_UPDATELN("\tAccess point started with SSID '%s' and password '%s'.", WIFI_AP_SSID, WIFI_AP_PASS);
    Serial.println();
    PRINT_UPDATELN("Attempting to attach servo to pin D4...");
    if (!servo.attach(D4)) {
        PRINT_UPDATELN("\tFailed to attach servo to pin D4");
    } else {
        PRINT_UPDATELN("\tAttached servo to pin D4.");
    }
    Serial.println();
    PRINT_UPDATELN("Initializing server...");
    IPAddress ip = WiFi.softAPIP();
    PRINT_UPDATELN("\tAdding anonymous callback for '/' client HTTP GET requests to send\n"
                   "\ttext/html webpage...");
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *req) {
        req->send(200, "text/html", INDEX_HTML);
    });
    server.on("/stopServo", HTTP_GET, [](AsyncWebServerRequest *req) {
        servo.stop();
        req->send(200, "text/plain", "Servo stopped.");
    });
    server.on("/startServo", HTTP_GET, [](AsyncWebServerRequest *req) {
        servo.start();
        req->send(200, "text/plain", "Servo started.");
    });
    server.on("/currentConfig", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "application/json", processor.getConfigDoc());
    });
    PRINT_UPDATELN("\tAdding callback for WebSocket events...");
    ws.onEvent(&websocketCallback);
    PRINT_UPDATELN("\tAdding WebSocket handler to server...");
    server.addHandler(&ws);
    PRINT_UPDATELN("\tServer initialized.");
    Serial.println();
    PRINT_UPDATELN("Starting server...");
    server.begin();
    PRINTF_UPDATELN("\tServer hosted on http://%s/", ip.toString().c_str());
    Serial.println();
}
void loop() {
    while (!receivedCommands.empty()) {
        if (!processor.handle(receivedCommands.front())) {

        }
        receivedCommands.pop();
    }
}