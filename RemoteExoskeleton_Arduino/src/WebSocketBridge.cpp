#include "WebSocketBridge.h"
#include "ServoController.h"

WebSocketBridge::WebSocketBridge() : server_(80),
ws_{"/ws"},
servo_{D4},
sampler_({std::nullopt, std::nullopt, std::nullopt, std::nullopt}, 100000UL),
verbosePrint(false) {}


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
    ws_.onEvent([this](AsyncWebSocket *s, AsyncWebSocketClient *c, AwsEventType t, void *a, uint8_t *d, size_t l) {
        this->onWsEvent(s, c, t, a, d, l);
    });


    servo_.setup();
    server_.begin();
    ws_.enable(true);

    servo_.addAngleNotify([this](int a) {emitServoAngle(a); });
    sampler_.onSample([this](const SampleSet &v) {emitSensorReadings(v); });
    if (!sampler_.begin()) {
        Serial << "Failed to start flex sensor sampler." << endl;
    }
}

void WebSocketBridge::loop() {
    // clean up disconnected clients
    while (!received.empty()) {
        auto request = received.front();
        handleReceived(request.c_str());
        received.pop();
    }
    ws_.cleanupClients();
    servo_.loop();
    sampler_.service();
    delay(1);
}

void WebSocketBridge::sendInvalidRequest(AsyncWebSocketClient *client) {
    char buf[200];

    outBuffer.clear();
    outBuffer["error"] = "Invalid request";
    outBuffer["details"] = inBuffer["req"].as<const char *>() != nullptr ? inBuffer["req"].as<const char *>() : "null";
    const size_t n = serializeJson(outBuffer, buf);
    client->text(buf, n);
    Serial << "Sent invalid request: " << inBuffer["req"].as<const char *>() << endl;
}
void WebSocketBridge::sendInvalidAttr(AsyncWebSocketClient *client) {
    outBuffer.clear();
    char buf[200];
    outBuffer["error"] = "Invalid attribute";
    outBuffer["details"] = inBuffer["attr"].as<const char *>() != nullptr ? inBuffer["attr"].as<const char *>() : "null";
    const size_t n = serializeJson(outBuffer, buf);
    client->text(buf, n);
    Serial << "Sent invalid attribute: " << inBuffer["attr"].as<const char *>() << endl;
}

void WebSocketBridge::sendSetResponse(AsyncWebSocketClient *client, const Status &status) {
    outBuffer.clear();
    outBuffer["stat"] = status == OK ? "OK" : "ERROR";
    char buf[200];
    size_t n = serializeJson(outBuffer, buf);
    client->text(buf, n);
    Serial << "Sent set response: \n >> " << buf << endl;
}

template <typename T>
void WebSocketBridge::sendGetResponse(const char *device, const char *attr, const T &val) {
    outBuffer.clear();
    outBuffer["dev"] = device;
    outBuffer["attr"] = attr;
    outBuffer["val"] = val;
    char buf[200];
    const size_t n = serializeJson(outBuffer, buf);
    ws_.textAll(buf, n);
    Serial << "Sent get response: " << val << endl;
}

void WebSocketBridge::emitServoAngle(int angle) {
    char buf[200];
    outBuffer.clear();
    outBuffer["dev"] = "SERVO";
    outBuffer["attr"] = "POSITION";
    outBuffer["val"] = angle;
    const size_t n = serializeJson(outBuffer, buf);
    ws_.textAll(buf, n);
}

void WebSocketBridge::emitSensorReadings(const SampleSet &readings) {
    outBuffer.clear();
    outBuffer["dev"] = "FLEX_2";
    outBuffer["attr"] = "READ";
    outBuffer["val"] = readings[0];
    char buff[200];
    size_t n = serializeJson(outBuffer, buff);
    ws_.textAll(buff, n);

    outBuffer.clear();
    outBuffer["dev"] = "FLEX_3";
    outBuffer["attr"] = "READ";
    outBuffer["val"] = readings[1];
    n = serializeJson(outBuffer, buff);
    ws_.textAll(buff, n);

    outBuffer.clear();
    outBuffer["dev"] = "FLEX_4";
    outBuffer["attr"] = "READ";
    outBuffer["val"] = readings[2];
    n = serializeJson(outBuffer, buff);
    ws_.textAll(buff, n);

    outBuffer.clear();
    outBuffer["dev"] = "FLEX_5";
    outBuffer["attr"] = "READ";
    outBuffer["val"] = readings[3];
    n = serializeJson(outBuffer, buff);
    ws_.textAll(buff, n);
}
WebSocketBridge::FlexAttr WebSocketBridge::parseFlexAttr() {
    if (inBuffer["attr"].isNull()) return FlexAttr::INVALID_FLEX_ATTR;
    auto retrieved = inBuffer["attr"].as<const char *>();
    if (strcmp(retrieved, "SAMPLE_RATE") == 0) {
        return FlexAttr::SampleRate;
    }
    if (strcmp(retrieved, "START") == 0) {
        return FlexAttr::Start;
    }
    if (strcmp(retrieved, "STOP") == 0) {
        return FlexAttr::Stop;
    }
    return FlexAttr::INVALID_FLEX_ATTR;
}
WebSocketBridge::FlexNAttr WebSocketBridge::parseFlexNAttr() {
    if (inBuffer["attr"].isNull()) return FlexNAttr::INVALID_FLEX_N_ATTR;

    if (strcmp(inBuffer["attr"].as<const char *>(), "PIN") == 0) {
        return FlexNAttr::Pin;
    }
    return FlexNAttr::INVALID_FLEX_N_ATTR;
}
WebSocketBridge::Device WebSocketBridge::parseDevice() {
    if (inBuffer["dev"].isNull()) return Device::INVALID_DEV;
    if (strcmp(inBuffer["dev"], "SERVO") == 0) {
        return Device::Servo;
    }
    if (strcmp(inBuffer["dev"], "FLEX") == 0) {
        return Device::Flex;
    }
    if (strcmp(inBuffer["dev"], "FLEX_2") == 0) {
        return Device::Flex_2;
    }
    if (strcmp(inBuffer["dev"], "FLEX_3") == 0) {
        return Device::Flex_3;
    }
    if (strcmp(inBuffer["dev"], "FLEX_4") == 0) {
        return Device::Flex_4;
    }
    if (strcmp(inBuffer["dev"], "FLEX_5") == 0) {
        return Device::Flex_5;
    }
    return Device::INVALID_DEV;
}
WebSocketBridge::Method WebSocketBridge::parseMethod() {
    if (inBuffer["req"].isNull()) return Method::INVALID_METHOD;
    if (strcmp(inBuffer["req"].as<const char *>(), "GET") == 0) return Method::GET;
    if (strcmp(inBuffer["req"], "SET") == 0) return Method::SET;
    return Method::INVALID_METHOD;
}
WebSocketBridge::ServoAttr WebSocketBridge::parseServoAttr() {
    if (inBuffer["attr"].isNull()) return ServoAttr::INVALID_SERVO_ATTR;
    auto attr = inBuffer["attr"].as<const char *>();
    if (strcmp(attr, "ANGLE_STEP") == 0) return ServoAttr::AngleStep;
    if (strcmp(attr, "TIME_DELAY") == 0) return ServoAttr::TimeDelayUS;
    if (strcmp(attr, "MIN_PWM") == 0) return ServoAttr::MinPWM;
    if (strcmp(attr, "MAX_PWM") == 0) return ServoAttr::MaxPWM;
    if (strcmp(attr, "POSITION") == 0) return ServoAttr::Position;
    if (strcmp(attr, "PIN") == 0) return ServoAttr::Pin;
    if (strcmp(attr, "ACTUATE") == 0) return ServoAttr::Actuate;
    if (strcmp(attr, "START_ANGLE") == 0) return ServoAttr::StartAngle;
    if (strcmp(attr, "STOP_ANGLE") == 0) return ServoAttr::StopAngle;
    if (strcmp(attr, "MOTION") == 0) return ServoAttr::Motion;
    return ServoAttr::INVALID_SERVO_ATTR;
}


void WebSocketBridge::onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT: {
            handleConnect(client);
        } break;
        case WS_EVT_DISCONNECT: {
            Serial << "Client disconnected." << endl;
            sampler_.stop();
            servo_.disableMotion();
        } break;
        case WS_EVT_DATA: {
            std::string msg(reinterpret_cast<const char *>(data), len);
            received.push(msg);
        } break;
        case WS_EVT_PONG:
            ws_.pingAll();
        default: break;
    }
}

void WebSocketBridge::handleConnect(AsyncWebSocketClient *client) {
    Serial << "Client 1/1 connected. Sending current information." << endl;
    sendGetResponse("SERVO", "POSITION", servo_.getPosition());
    sendGetResponse("SERVO", "MOTION", ServoController::motionString(servo_.getMotion()));
    sendGetResponse( "SERVO", "TIME_DELAY", servo_.getTimeDelay());
    sendGetResponse( "SERVO", "ANGLE_STEP", servo_.getAngleStep());
    sendGetResponse( "SERVO", "MIN_PWM", servo_.getPwmMin());
    sendGetResponse( "SERVO", "MAX_PWM", servo_.getPwmMax());
    sendGetResponse("SERVO", "PIN", getRemap(servo_.getPin()));
    sendGetResponse( "SERVO", "START_ANGLE", servo_.getStartAngle());
    sendGetResponse( "SERVO", "STOP_ANGLE", servo_.getStopAngle());
    sendGetResponse( "FLEX", "SAMPLE_RATE", sampler_.getSamplingRate());
    sendGetResponse("FLEX_2", "PIN", sampler_.getPin(SensorSampler::INDEX).value_or(false));
    sendGetResponse("FLEX_3", "PIN", sampler_.getPin(SensorSampler::MIDDLE).value_or(false));
    sendGetResponse("FLEX_4", "PIN", sampler_.getPin(SensorSampler::RING).value_or(false));
    sendGetResponse("FLEX_5", "PIN", sampler_.getPin(SensorSampler::PINKY).value_or(false));

}


void WebSocketBridge::handleReceived(const char *request) {
    inBuffer.clear();
    DeserializationError error = deserializeJson(inBuffer, request);
    if (error) {
        Serial << "Failed to parse request: " << error.c_str() << endl;
        return;
    }
    auto dev = parseDevice();
    auto req = parseMethod();
    if (dev != Device::INVALID_DEV && req != Method::INVALID_METHOD) {
        switch (dev) {
            case Device::Servo: {
                auto attr = parseServoAttr();
                if (req == Method::SET && !inBuffer["val"].isNull()) {
                    switch (attr) {
                        case ServoAttr::AngleStep:
                            servo_.setAngleStep(inBuffer["val"].as<int>());
                            break;
                        case ServoAttr::TimeDelayUS:
                            servo_.setTimeDelay(inBuffer["val"].as<int>());
                            break;
                        case ServoAttr::MinPWM:
                            servo_.setMaxPWM(inBuffer["val"].as<int>());
                            break;
                        case ServoAttr::MaxPWM:
                            servo_.setMinPWM(inBuffer["val"].as<int>());
                            break;
                        case ServoAttr::Position:
                            servo_.setPosition(inBuffer["val"].as<unsigned int>());
                            break;
                        case ServoAttr::Pin:
                            servo_.setPin(inBuffer["val"].as<uint8_t>());
                            break;
                        case ServoAttr::Actuate: {
                            auto enabled = inBuffer["val"].as<bool>();
                            if (enabled == true) {
                                servo_.enableMotion();
                            } else {
                                servo_.disableMotion();
                            }
                        } break;
                        case ServoAttr::StartAngle: {
                            servo_.setStartAngle(inBuffer["val"].as<unsigned int>());
                        } break;
                        case ServoAttr::StopAngle: {
                            servo_.setStopAngle(inBuffer["val"].as<unsigned int>());
                        } break;
                        case ServoAttr::Motion:
                            servo_.setMotion(ServoController::fromString( inBuffer["val"].as<const char *>()));
                            break;
                        default: {
                            Serial << "Invalid servo attribute: " << inBuffer["val"].as<const char *>() << endl;
                            sendInvalidAttr(&ws_.getClients().front());
                        } break;
                    }
                }
                else if (req == Method::GET) {
                    switch (attr) {
                        case ServoAttr::AngleStep:
                            sendGetResponse("SERVO", "ANGLE_STEP", servo_.getAngleStep());
                            break;
                        case ServoAttr::TimeDelayUS:
                            sendGetResponse("SERVO", "TIME_DELAY", servo_.getTimeDelay());
                            break;
                        case ServoAttr::MinPWM:
                            sendGetResponse("SERVO", "MIN_PWM", servo_.getPwmMin());
                            break;
                        case ServoAttr::MaxPWM:
                            sendGetResponse( "SERVO", "MAX_PWM", servo_.getPwmMax());
                            break;
                        case ServoAttr::Position:
                            sendGetResponse("SERVO", "POSITION", servo_.getPosition());
                            break;
                        case ServoAttr::Pin:
                            sendGetResponse("SERVO", "PIN", getRemap(servo_.getPin()));
                            break;
                        case ServoAttr::Actuate:
                            sendGetResponse( "SERVO", "ACTUATE", servo_.isActive());
                            break;
                        case ServoAttr::StartAngle:
                            sendGetResponse( "SERVO", "START_ANGLE", servo_.getStartAngle());
                            break;
                        case ServoAttr::StopAngle:
                            sendGetResponse( "SERVO", "STOP_ANGLE", servo_.getStopAngle());
                            break;
                        case ServoAttr::Motion:
                            sendGetResponse( "SERVO", "MOTION", ServoController::motionString(servo_.getMotion()));
                            break;
                        default:
                            sendInvalidAttr(&ws_.getClients().front());
                    }
                }
                else {
                    // clamp to buffer size minus one for null-term
                    char buff[200];
                    size_t toCopy = (sizeof(request) < 199) ? sizeof(request) : 199;
                    memcpy(buff, request, toCopy);
                    buff[toCopy] = '\0';
                    sendInvalidRequest(&ws_.getClients().front());
                }
            } break;
            case Device::Flex: {
                auto attr = parseFlexAttr();
                if (attr != FlexAttr::INVALID_FLEX_ATTR) {
                    if (attr == FlexAttr::SampleRate) {
                        if (req == Method::SET) {
                            if (inBuffer["val"].isNull()) {
                                sendInvalidAttr(&ws_.getClients().front());
                            } else {
                                sampler_.setSamplingRate(inBuffer["val"].as<unsigned int>());
                            }
                        } else {
                            sendGetResponse("FLEX", "SAMPLE_RATE", sampler_.getSamplingRate());
                        }
                    } else if (attr == FlexAttr::Start) {
                        sampler_.start();
                        sendSetResponse(&ws_.getClients().front(), OK);
                    } else {
                        sampler_.stop();
                        sendSetResponse(&ws_.getClients().front(), OK);
                    }
                } else {
                    sendInvalidAttr(&ws_.getClients().front());
                }
            } break;
            case Device::Flex_2: {
                auto attr = parseFlexNAttr();
                bool notConnected = inBuffer["val"].is<bool>();
                if (attr == FlexNAttr::Pin) {
                    if (req == Method::GET) {
                        sendGetResponse("FLEX_2", "PIN", sampler_.getPin(SensorSampler::INDEX).value_or(false));
                    } else {
                        if (notConnected) {
                            sampler_.setPin(SensorSampler::INDEX, std::nullopt);
                        } else {
                            sampler_.setPin(SensorSampler::INDEX, inBuffer["val"].as<uint8_t>());
                        }
                    }
                } else {
                    sendInvalidAttr(&ws_.getClients().front());
                }
            } break;
            case Device::Flex_3: {
                auto attr = parseFlexNAttr();
                bool notConnected = inBuffer["val"].is<bool>();
                if (attr == FlexNAttr::Pin) {
                    if (req == Method::GET) {
                        sendGetResponse("FLEX_3", "PIN", sampler_.getPin(SensorSampler::MIDDLE).value_or(false));
                    } else {
                        if (notConnected) {
                            sampler_.setPin(SensorSampler::MIDDLE, std::nullopt);
                        } else {
                            sampler_.setPin(SensorSampler::MIDDLE, inBuffer["val"].as<uint8_t>());
                        }
                    }
                } else {
                    sendInvalidAttr(&ws_.getClients().front());
                }
            } break;
            case Device::Flex_4: {
                auto attr = parseFlexNAttr();
                bool notConnected = inBuffer["val"].is<bool>();
                if (attr == FlexNAttr::Pin) {
                    if (req == Method::GET) {
                        sendGetResponse("FLEX_4", "PIN", sampler_.getPin(SensorSampler::RING).value_or(false));
                    } else {
                        if (notConnected) {
                            sampler_.setPin(SensorSampler::RING, std::nullopt);
                        } else {
                            sampler_.setPin(SensorSampler::RING, inBuffer["val"].as<uint8_t>());
                        }
                    }
                } else {
                    sendInvalidAttr(&ws_.getClients().front());
                }
            } break;
            case Device::Flex_5: {
                auto attr = parseFlexNAttr();
                bool notConnected = inBuffer["val"].is<bool>();
                if (attr == FlexNAttr::Pin) {
                    if (req == Method::GET) {
                        sendGetResponse("FLEX_5", "PIN", sampler_.getPin(SensorSampler::PINKY).value_or(false));
                    } else {
                        if (notConnected) {
                            sampler_.setPin(SensorSampler::PINKY, std::nullopt);
                        } else {
                            sampler_.setPin(SensorSampler::PINKY, inBuffer["val"].as<uint8_t>());
                        }
                    }
                } else {
                    sendInvalidAttr(&ws_.getClients().front());
                }
                } break;
            default:
                break;
        }
    } else {
        sendInvalidRequest(&ws_.getClients().front());
    }
}

