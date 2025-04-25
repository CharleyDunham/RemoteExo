#ifndef WEBSOCKET_BRIDGE_H
#define WEBSOCKET_BRIDGE_H

#pragma once
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "ServoController.h"
#include "SensorSampler.h"
#include "SerialStream.h"
#include <WiFi.h>
#include <FS.h>
#include <SPIFFS.h>
#include <queue>
#include <io_pin_remap.h>
#ifndef BOARD_HAS_PIN_REMAP
    #define BOARD_HAS_PIN_REMAP
#endif

class WebSocketBridge {
public:
    WebSocketBridge();

    // call once in setup()
    void begin();

    // call every loop()
    void loop();

private:
    AsyncWebSocket       ws_;
    ServoController     servo_;
    SensorSampler       sampler_;
    AsyncWebServer      server_;
    JsonDocument        inBuffer;
    JsonDocument        outBuffer;


    bool verbosePrint;
    // WebSocket event handler
    void onWsEvent(AsyncWebSocket *server,
                   AsyncWebSocketClient *client,
                   AwsEventType type,
                   void *arg,
                   uint8_t *data,
                   size_t len) ;

    // called whenever servo moves
    void emitServoAngle(int angle);

    // called whenever sampler has a batch
    void emitSensorReadings(const SampleSet& readings);
    enum Status { OK, ERROR };
    // called whenever an update occurs. Could be an error or not.
    void sendInvalidRequest(AsyncWebSocketClient *client);
    void sendSetResponse(AsyncWebSocketClient *client, const Status &status);
    void sendInvalidAttr(AsyncWebSocketClient *client);

    template <typename T>
    void sendGetResponse(const char *device, const char *attr, const T &val);
    enum class Device { Servo, Flex, Flex_2, Flex_3, Flex_4, Flex_5, INVALID_DEV };
    enum class Method { GET, SET, INVALID_METHOD };
    enum class ServoAttr { AngleStep, TimeDelayUS, MinPWM, MaxPWM, Position, Pin, Actuate, StartAngle, StopAngle, Motion, INVALID_SERVO_ATTR };
    enum class FlexAttr { SampleRate, Start, Stop, INVALID_FLEX_ATTR };
    enum class FlexNAttr { Pin, INVALID_FLEX_N_ATTR };
    void handleConnect(AsyncWebSocketClient *client);
    Device parseDevice();
    Method parseMethod();
    ServoAttr parseServoAttr();
    FlexAttr parseFlexAttr();
    FlexNAttr parseFlexNAttr();
    // attr
    std::queue<std::string> received;
    void handleReceived(const char *message);
    //static FlexNAttr parseFlexNAttr(const char *attr);
};
#endif