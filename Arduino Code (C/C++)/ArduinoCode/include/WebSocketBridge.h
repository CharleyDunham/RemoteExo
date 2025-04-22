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
    static void sendInvalidRequest(AsyncWebSocketClient *client, const char *request);
    static void sendSetResponse(AsyncWebSocketClient *client, const JsonDocument &requestDoc, const Status &status);
    static void sendInvalidAttr(AsyncWebSocketClient *client, const char *attr);
    template <typename T>
    static void sendGetResponse(AsyncWebSocketClient *client, const char *device, const char *attr, const T &val);
    enum class Device { Servo, Flex, FlexN, UnknownDev };
    enum class Method { Get, Set, UnknownMethod };
    enum class ServoAttr { AngleStep, TimeDelayUS, MinPWM, MaxPWM, Position, Pin, Start, Stop, StartAngle, StopAngle, Motion, UnknownServoAttribute };
    enum class FlexAttr { SampleRate, UnknownFlexAttribute };
    enum class FlexNAttr { Pin, UnknownFlexNAttribute };

    static Device parseDevice(const char *device);
    static Method parseMethod(const char *method);
    static ServoAttr parseServoAttr(const char *attr);
    static FlexAttr parseFlexAttr(const char *attr);
    // attr
    static FlexNAttr parseFlexNAttr(const char *attr);
};
#endif