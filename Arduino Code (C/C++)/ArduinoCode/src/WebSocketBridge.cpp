#include "WebSocketBridge.h"
#include "Servo.h"

WebSocketBridge::WebSocketBridge() : server_(80),
ws_{"/ws"},
servo_{D4},
sampler_({A0, A1, A2, A3}, 100000),
verbosePrint(false)
{}


void WebSocketBridge::begin() {
    if (!SPIFFS.begin(true)) throw std::runtime_error("Failed to mount SPIFFS");
    WiFi.mode(WIFI_AP);
    WiFi.softAP("RemoteExoskeleton", "remoteExoskeleton");
    server_.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
    server_.serveStatic("/script.js", SPIFFS, "/script.js");
    server_.serveStatic("/style.css", SPIFFS, "/style.css");
    server_.onNotFound([](auto *req) {
        req->send(404);
    });
    server_.addHandler(&ws_);
    ws_.onEvent([this](auto *s, auto *c, auto t, auto *a, auto *d, auto l) {
        onWsEvent(s, c, t, a, d, l);
    });
    servo_.setup();
    server_.begin();
    ws_.enable(true);
    servo_.onAngle([this](int a) {emitServoAngle(a); });
    sampler_.onSample([this](const SampleSet &v) {emitSensorReadings(v); });
    if (!sampler_.begin()) {
        Serial << "Failed to start flex sensor sampler." << endl;
    }
}

void WebSocketBridge::loop() {
    // clean up disconnected clients
    ws_.cleanupClients();
    servo_.loop();
    sampler_.service();
}

void WebSocketBridge::sendInvalidRequest(AsyncWebSocketClient *client, const char *request) {
    JsonDocument response;
    char buf[64];
    response["error"] = "Invalid request";
    response["details"] = request;
    const size_t n = serializeJson(response, buf);
    client->text(buf, n);
    Serial << "Sent invalid request: " << request << endl;
}
void WebSocketBridge::sendInvalidAttr(AsyncWebSocketClient *client, const char *attr) {
    JsonDocument response;
    char buf[64];
    response["error"] = "Invalid attribute";
    response["details"] = attr;
    const size_t n = serializeJson(response, buf);
    client->text(buf, n);
    Serial << "Sent invalid attribute: " << attr << endl;
}

void WebSocketBridge::sendSetResponse(AsyncWebSocketClient *client, const JsonDocument &requestDoc, const Status &status) {
    JsonDocument response = requestDoc;
    response["stat"] = status == OK ? "OK" : "ERROR";
    char buf[128];
    size_t n = serializeJson(response, buf);
    client->text(buf, n);
    Serial << "Sent set response: \n >> " << buf << endl;
}

template <typename T>
void WebSocketBridge::sendGetResponse(AsyncWebSocketClient *client, const char *device, const char *attr, const T &val) {
    JsonDocument response;
    response["dev"] = device;
    response["attr"] = attr;
    response["val"] = val;
    char buf[128];
    const size_t n = serializeJson(response, buf);
    client->text(buf, n);
    Serial << "Sent get response: " << val << endl;
}

void WebSocketBridge::emitServoAngle(int angle) {
    JsonDocument doc;
    doc["type"] = "servo";
    doc["angle"] = angle;
    char buf[64];
    const size_t n = serializeJson(doc, buf);
    for (auto &c: ws_.getClients())
        c.text(buf, n);
}

void WebSocketBridge::emitSensorReadings(const SampleSet &readings) {
    JsonDocument doc;
    doc["type"] = "flex";
    auto arr = doc["values"].to<JsonArray>();
    for (auto v: readings) arr.add(v);

    char buf[128];
    size_t n = serializeJson(doc, buf);
    for (auto &c: ws_.getClients())
        c.text(buf, n);
}
WebSocketBridge::FlexAttr WebSocketBridge::parseFlexAttr(const char *attr) {
    if (strcmp(attr, "SAMPLE_RATE") == 0) {
        return FlexAttr::SampleRate;
    }
    return FlexAttr::UnknownFlexAttribute;
}
WebSocketBridge::FlexNAttr WebSocketBridge::parseFlexNAttr(const char *attr) {
    if (strcmp(attr, "PIN") == 0) {
        return FlexNAttr::Pin;
    }
    return FlexNAttr::UnknownFlexNAttribute;
}
WebSocketBridge::Device WebSocketBridge::parseDevice(const char *device) {
    if (strcmp(device, "SERVO") == 0) return Device::Servo;
    if (strcmp(device, "FLEX") == 0) return Device::Flex;
    if (strncmp(device, "FLEX-", 5) == 0) return Device::FlexN;
    return Device::UnknownDev;
}
WebSocketBridge::Method WebSocketBridge::parseMethod(const char *method) {
    if (strcmp(method, "GET") == 0) return Method::Get;
    if (strcmp(method, "SET") == 0) return Method::Set;
    return Method::UnknownMethod;
}
WebSocketBridge::ServoAttr WebSocketBridge::parseServoAttr(const char *attr) {
    if (strcmp(attr, "ANGLE_STEP") == 0) return ServoAttr::AngleStep;
    if (strcmp(attr, "TIME_DELAY") == 0) return ServoAttr::TimeDelayUS;
    if (strcmp(attr, "MIN_PWM") == 0) return ServoAttr::MinPWM;
    if (strcmp(attr, "MAX_PWM") == 0) return ServoAttr::MaxPWM;
    if (strcmp(attr, "POSITION") == 0) return ServoAttr::Position;
    if (strcmp(attr, "PIN") == 0) return ServoAttr::Pin;
    if (strcmp(attr, "START") == 0) return ServoAttr::Start;
    if (strcmp(attr, "STOP") == 0) return ServoAttr::Stop;
    if (strcmp(attr, "START_ANGLE") == 0) return ServoAttr::StartAngle;
    if (strcmp(attr, "STOP_ANGLE") == 0) return ServoAttr::StopAngle;
    if (strcmp(attr, "MOTION") == 0) return ServoAttr::Motion;
    return ServoAttr::UnknownServoAttribute;
}


void WebSocketBridge::onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)  {
    if (type == WS_EVT_CONNECT) {
        sendGetResponse(client, "SERVO", "MOTION", servo_.getCurrentMode());
        sendGetResponse(client, "SERVO", "POSITION", servo_.getPosition());
        sendGetResponse(client, "SERVO", "PIN", servo_.getPin());
        sendGetResponse(client, "SERVO", "START_ANGLE", servo_.getStartAngle());
        sendGetResponse(client, "SERVO", "STOP_ANGLE", servo_.getStopAngle());
        sendGetResponse(client, "SERVO", "FREQUENCY", 50);
        sendGetResponse(client, "SERVO", "ANGLE_STEP", servo_.getAngleStep());
        sendGetResponse(client, "SERVO", "MIN_PWM", servo_.getMinPWM());
        sendGetResponse(client, "SERVO", "MAX_PWM", servo_.getMaxPWM());
        sendGetResponse(client, "SERVO", "TIME_DELAY", servo_.getTimeDelayUS());
        sendGetResponse(client, "FLEX", "SAMPLING_RATE", sampler_.getSamplingRate());
        sendGetResponse(client, "FLEX-1", "PIN", "A0");
        return;
    }
    if (type == WS_EVT_DISCONNECT) {
        return ws_.cleanupClients();
    }
    if (type != WS_EVT_DATA) return;

    JsonDocument doc;
    JsonDocument response;
    DeserializationError error = deserializeJson(doc, data);
    if (error) {
        Serial << "Failed to deserialize JSON: " << error.c_str() << endl;
        return;
    }

    auto dev = parseDevice(doc["dev"].as<const char*>());
    auto method = parseMethod(doc["req"].as<const char*>());
    auto sAttr = doc["attr"].as<const char*>();

    switch (dev) {
        case Device::Servo: {
            auto sa = parseServoAttr(sAttr);
            if (method == Method::Get) {
                switch (sa) {
                    case ServoAttr::AngleStep:
                        sendGetResponse(client, "SERVO", "ANGLE_STEP", servo_.getAngleStep());
                        Serial << "Sent get response for angle-step." << endl;
                        break;
                    case ServoAttr::TimeDelayUS:
                        sendGetResponse(client, "SERVO", "TIME_DELAY_US", servo_.getTimeDelayUS());
                        Serial << "Sent get response for time-delay." << endl;
                        break;
                    case ServoAttr::MinPWM:
                        sendGetResponse(client, "SERVO", "MIN_PWM", servo_.getMinPWM());
                        Serial << "Sent get response for min-pwm." << endl;
                        break;
                    case ServoAttr::MaxPWM:
                        sendGetResponse(client, "SERVO", "MAX_PWM", servo_.getMaxPWM());
                        Serial << "Sent get response for max-pwm." << endl;
                        break;
                    case ServoAttr::Position:
                        sendGetResponse(client, "SERVO", "POSITION", servo_.getPosition());
                        Serial << "Sent get response for set-angle." << endl;
                        break;
                    case ServoAttr::Pin:
                        sendGetResponse(client, "SERVO", "PIN", servo_.getPin());
                        Serial << "Sent get response for pin." << endl;
                        break;
                    case ServoAttr::StartAngle:
                        sendGetResponse(client, "SERVO", "START_ANGLE", servo_.getStartAngle());
                        break;
                    case ServoAttr::StopAngle:
                        sendGetResponse(client, "SERVO", "STOP_ANGLE", servo_.getStopAngle());
                        break;
                    case ServoAttr::Motion:
                        sendGetResponse(client, "SERVO", "MOTION", ServoController::modeStr(servo_.getCurrentMode()));
                        break;
                    default:
                        sendInvalidAttr(client, doc["attr"].as<const char*>());
                        break;
                }
            } else if (method == Method::Set) {
                switch (sa) {
                    case ServoAttr::AngleStep:
                        try {
                            servo_.setAngleStep(doc["val"].as<int>());
                            sendSetResponse(client, doc, OK);
                        } catch (std::invalid_argument &e) {
                            Serial << "Invalid angle-step received: " << doc["Value"].as<const char*>() << endl;
                            sendSetResponse(client, doc, ERROR);
                        }
                        break;
                    case ServoAttr::TimeDelayUS:
                        try {
                            if (!doc["val"].is<unsigned long>()) {
                                throw std::invalid_argument("Invalid time-delay received");
                            }
                            servo_.setTimeDelayUS(doc["val"].as<unsigned long>());
                            Serial << "Set servo time-delay to " << doc["Value"].as<unsigned long>() << endl;
                            return sendSetResponse(client, doc, OK);
                        } catch (std::invalid_argument &e) {
                            Serial << "Invalid time-delay request: " << doc["Value"].as<unsigned long>() << endl;
                            return sendSetResponse(client, doc, ERROR);
                        }
                    case ServoAttr::MinPWM:
                        try {
                            if (!doc["val"].is<int>()) {
                                throw std::invalid_argument("Invalid min-pwm value");
                            }
                            servo_.setMinPWM(doc["val"].as<int>());
                            return sendSetResponse(client, doc, OK);
                        } catch (std::invalid_argument &) {
                            Serial << "Invalid min-pwm request: " << doc["val"].as<int>() << endl;
                            return sendSetResponse(client, doc, ERROR);
                        }
                    case ServoAttr::MaxPWM:
                        try {
                            if (!doc["val"].is<int>()) {
                                throw std::invalid_argument("Invalid max-pwm request");
                            }
                            servo_.setMaxPWM(doc["val"].as<int>());
                            Serial << "Set max-pwm to " << servo_.getMaxPWM() << endl;
                            return sendSetResponse(client, doc, OK);
                        } catch (std::invalid_argument &) {
                            Serial << "Invalid max-pwm request: " << doc["val"].as<int>() << endl;
                            return sendSetResponse(client, doc, ERROR);
                        }
                    case ServoAttr::Position:
                        try {
                            servo_.setPosition(doc["val"].as<unsigned int>());
                            Serial << "Set servo angle to " << doc["val"].as<unsigned int>() << endl;
                            return sendSetResponse(client, doc, OK);
                        } catch (std::invalid_argument &) {
                            Serial << "Invalid set-angle request: " << doc["val"].as<unsigned int>() << endl;
                            return sendSetResponse(client, doc, ERROR);
                        }
                    case ServoAttr::Pin:
                        if (doc["val"].is<uint8_t>()) {
                            Serial << "Updated servo pin to " << doc["val"].as<uint8_t>() << endl;
                            servo_.setPin(doc["val"].as<uint8_t>());
                            return sendSetResponse(client, doc, OK);
                        }
                        Serial << "Value received  not uint8_t for SET_PIN request" << endl;
                        return sendSetResponse(client, doc, ERROR);
                    case ServoAttr::Start:
                        try {
                            servo_.start();
                            Serial << "Sent OK to start servo." << endl;
                            return sendSetResponse(client, doc, OK);
                        } catch (std::runtime_error &e) {
                            Serial << "Error starting servo: " << e.what() << endl;
                            return sendSetResponse(client, doc, ERROR);
                            //esp_restart();
                        }
                    case ServoAttr::Stop:
                        servo_.stop();
                        Serial << "Stopped servo" << endl;
                        return sendSetResponse(client, doc, OK);
                    case ServoAttr::StartAngle:
                        try {
                            servo_.setStartAngle(doc["val"].as<int>());
                            Serial << "Set new start angle " << servo_.getStartAngle() << endl;
                            return sendSetResponse(client, doc, OK);
                        } catch (std::exception &e) {
                            Serial << "Error setting start angle: " << e.what() << endl;
                            return sendInvalidAttr(client, doc["attr"].as<const char*>());
                        }
                    case ServoAttr::StopAngle:
                        try {
                            if (!doc["val"].is<uint8_t>()) {
                                throw std::invalid_argument("Invalid stop-angle request");
                            }
                            servo_.setStopAngle(doc["val"].as<int>());
                            Serial << "Set new stop angle " << servo_.getStopAngle() << endl;
                            return sendSetResponse(client, doc, OK);
                        } catch (std::exception &e) {
                            Serial << "Error setting stop angle: " << e.what() << endl;
                            return sendInvalidAttr(client, doc["attr"].as<const char*>());
                        }
                    case ServoAttr::Motion:
                    {
                        auto mode = ServoController::getModeName(doc["val"].as<const char*>());
                        if (mode != ServoController::Mode::INVALID) {
                            servo_.setMode(mode);
                            return sendSetResponse(client, doc, OK);
                        }
                        return sendInvalidAttr(client, doc["val"].as<const char*>());
                    }
                    default:
                        return sendInvalidAttr(client, doc["attr"].as<const char*>());
                }
            }
            if (doc["req"].is<const char *>()) {
                return sendInvalidRequest(client, doc["req"].as<const char*>());
            }
            return sendInvalidRequest(client, "Unknown request");
        }
        case Device::Flex: {
            if (method == Method::Get) {
                if (strcmp(doc["attr"].as<const char *>(), "SAMPLE_RATE") == 0) {
                    Serial << "Sent get response for sample-rate." << endl;
                    return sendGetResponse(client, "FLEX", "SAMPLE_RATE", sampler_.getSamplingRate());
                }
                return sendInvalidAttr(client, doc["attr"].as<const char*>());
            }
            if (method == Method::Set) {
                if (strcmp(doc["attr"].as<const char *>(), "SAMPLE_RATE") == 0) {
                    try {
                        sampler_.setSamplingRate(doc["val"].as<unsigned long>());
                        return sendSetResponse(client, doc, OK);
                    } catch (std::invalid_argument &) {
                        return sendInvalidRequest(client, "Sampling rate is too frequent.");
                    }
                }
                if (strcmp(doc["attr"].as<const char *>(), "STREAM") == 0) {
                    if (!doc["val"].is<bool>()) {
                        return sendInvalidAttr(client, "Stream must be a boolean.");
                    }
                    bool stream = doc["val"].as<bool>();
                    if (stream) {
                        try {
                            sampler_.start();
                            Serial << "Started sampling flex sensors. " << endl;
                            return sendSetResponse(client, doc, OK);
                        } catch (std::runtime_error &e) {
                            Serial << "Error starting timer." << endl;
                            return sendSetResponse(client, doc, ERROR);
                        }
                    }
                    sampler_.stop();
                    return sendSetResponse(client, doc, OK);
                }
                return sendInvalidAttr(client, doc["attr"].as<const char*>());
            }
            return sendInvalidRequest(client, "Unknown method.");
        }
        case Device::FlexN: {
            // pull the “N” out of “FLEX-N”
            int idx = doc["dev"].as<const char*>()[5] - '1';
            if (idx<0||idx>3) return sendInvalidRequest(client,"Bad flex index");
            if (method==Method::Set && parseFlexNAttr(sAttr)==FlexNAttr::Pin) {
                // reuse the same setter signature you already have
                if (doc["val"].is<uint8_t>()) {
                    sampler_.setPin(idx, doc["val"].as<uint8_t>());
                    return sendSetResponse(client, doc, OK);
                }
                if (doc["val"].is<bool>() ||
                    (doc["val"].is<const char*>() &&
                     strcmp(doc["val"].as<const char*>(),"NC")==0))
                {
                    sampler_.setPin(idx, SensorSampler::NOT_CONNECTED);
                    return sendSetResponse(client, doc, OK);
                }
                return sendSetResponse(client, doc, ERROR);
            }
            return sendInvalidAttr(client, sAttr);
        }
        default:
            return sendInvalidRequest(client, "Unknown device.");
    }
}