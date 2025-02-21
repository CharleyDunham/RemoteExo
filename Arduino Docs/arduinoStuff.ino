#include <WebSocketsClient_Generic.h>
#include <WiFiManager.h>
#include <WiFi.h>
#include <ArduinoJson.h>
/*
 *  Instantiate a global webSocket instance.
 */
WebSocketsClient webSocket;
/*
 *  Define parameters, i.e.,
 *    - port number: 443
 *    - path (current): /
 *    - host domain: (see below)
 */
const uint16_t port = 443;
const char *path = "/";
const char *host = "https://cc5d-146-70-147-101.ngrok-free.app";

/*
 *  Define callback for a webSocketEvent
 *     (defined by the library's WStype_t handler)
 *  Accepts the payload (pointer to character) and its length (size_t)
 */
void onWebSocketEvent(WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED:
    /*
       WebSocket connected event: just say hi from team 13
     */
      Serial.printf("\nconnected to ws server in domain %s", host);
      webSocket.sendTXT("hi from team 13");
      break;
    /*
       WS disconnected event: just inform myself through serial monitor
     */
    case WStype_DISCONNECTED:
      Serial.println("Disconnected from WebSocket server.");
      break;
    /*
       WS new message: will be deserialized
     */
    case WStype_TEXT:
    {
        Serial.print("Message from server: ");
        Serial.println("New command");
        Serial.println(*payload);
    } break;
    case WStype_ERROR:
      Serial.println("WebSocket Error");
      break;

    default:
      break;
  }
}


void setup(void) {
  // put your setup code here, to run once:
  WiFiManager wm;
  bool res = wm.autoConnect("RemoteExo_AP", "remoteExoskeleton");
  if (!res) {
    Serial.println("Failed to connect.");
  } else {
    webSocket.begin(host, port, path);
    webSocket.onEvent(onWebSocketEvent);
  }
}

void loop(void) {
  // put your main code here, to run repeatedly:
  webSocket.loop();
  static unsigned long lastMessageTime = 0;
  if (millis() - lastMessageTime > 5000) {
    lastMessageTime = millis();
    webSocket.sendTXT("Ping from Arduino");
  }
}
