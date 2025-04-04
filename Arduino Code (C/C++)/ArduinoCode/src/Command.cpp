//
// Created by Sullivan Bryant on 4/2/25.
//
#include "Command.h"


bool CommandProcessor::tryParse(const String &args, int &value) {
    if (args.equalsIgnoreCase("0")) {
        value = 0;
        return true;
    } else {
        int val = args.toInt();
        if (val != 0) {
            value = val;
            return true;
        } else {
            return false;
        }
    }
}
bool CommandProcessor::tryParse(const String &args, long &value) {
    if (args.equalsIgnoreCase("0")) {
        value = 0;
        return true;
    } else {
        long val = args.toInt();
        if (val != 0) {
            value = val;
            return true;
        } else {
            return false;
        }
    }
}
bool CommandProcessor::tryParse(const String &args, unsigned long &value) {
    if (args.equalsIgnoreCase("0")) {
        value = 0;
        return true;
    } else {
        long val = args.toInt();
        if (val > 0) {
            value = static_cast<unsigned long>(val);
            return true;
        } else {
            return false;
        }
    }
}
bool CommandProcessor::tryParse(const String &args, unsigned int &value) {
    if (args.equalsIgnoreCase("0")) {
        value = 0;
        return true;
    } else {
        long val = args.toInt();
        if (val > 0) {
            value = static_cast<unsigned int>(val);
            return true;
        } else {
            return false;
        }
    }
}

template <typename T>
void CommandProcessor::sendOK(AsyncWebSocketClient *client, const String &command, const T &arg) {
    if (client != nullptr) {
        responseDoc.clear();
        responseString.clear();
        responseDoc["device"] = "SERVO";
        responseDoc["request_made"] = command + String(arg);
        responseDoc["status"] = 200;
        serializeJsonPretty(responseDoc, responseString);
        client->text(responseString);
    } else {
        Serial.println("Invalid entry '" + String(arg) + "' for command '" + command + "'.");
    }
}

void CommandProcessor::sendOK(AsyncWebSocketClient *client, const String &command, const String &arg) {
    if (client != nullptr) {
        responseDoc.clear();
        responseString.clear();
        responseDoc["device"] = "SERVO";
        responseDoc["request_made"] = command + arg;
        responseDoc["status"] = 200;
        serializeJsonPretty(responseDoc, responseString);
        client->text(responseString);
    }
    if (arg.length() == 0) {
        PRINTF_UPDATELN("Success: '%s'.", command.c_str());
    } else {
        PRINTF_UPDATELN("Successfully %s to %s.", command.c_str(), arg.c_str());
    }
}

template <typename T>
void CommandProcessor::sendFailed(AsyncWebSocketClient *client, const String &command, const T &arg) {
    if (client != nullptr) {
        responseDoc.clear();
        responseString.clear();
        responseDoc["device"] = "SERVO";
        responseDoc["request_made"] = command + String(arg);
        responseDoc["status"] = 200;
        serializeJsonPretty(responseDoc, responseString);
        client->text(responseString);
    }
    Serial.println("Invalid entry '" + String(arg) + "' for command '" + command + "'.");
}

void CommandProcessor::sendFailed(AsyncWebSocketClient *client, const String &command, const String &arg) {
    if (client != nullptr) {
        responseDoc.clear();
        responseString.clear();
        responseDoc["device"] = "SERVO";
        responseDoc["request_made"] = command + arg;
        responseDoc["status"] = 200;
        serializeJsonPretty(responseDoc, responseString);
        client->text(responseString);
    }
    Serial.println("Invalid entry '" + arg + "' for command '" + command + "'.");
}
void CommandProcessor::sendFailed(const CommandInput &input) {
    if (input.client != nullptr) {
        responseDoc.clear();
        responseString.clear();
        responseDoc["device"] = "SERVO";
        responseDoc["request_made"] = "INVALID";
        responseDoc["status"] = 404;
        responseDoc["details"] = "Couldn't deserialize input received: " + input.raw;
        serializeJsonPretty(responseDoc, responseString);
        input.client->text(responseString);
    }
}

void CommandProcessor::onPrintCommands(AsyncWebSocketClient *client) {
    if (client == nullptr) {
        for (const auto &command: commands) {
            printFixed(command.first);
            printFixedln(command.second.commandDescription);
        }
    } else {
        responseDoc.clear();
        responseString.clear();
        responseDoc["device"] = "SERVO";
        responseDoc["request_made"] = "HELP";
        responseDoc["status"] = 200;
        for (const auto &command : commands) {
            responseDoc[command.first] = command.second.commandDescription;
        }
        serializeJsonPretty(responseDoc, responseString);
        client->text(responseString);
    }
}
CommandProcessor::CommandProcessor(ESPServo &servo) : servo(servo), printUpdates(false)
{
    commands = {
            {
                "setAngleStep",
                {
                    "Set angle-step of servo.",
                    [this](const String &arg, AsyncWebSocketClient *client) -> bool {
                        int val;
                        return tryParse(arg, val) && this->servo.setAngleStep(val);
                    }
                }
            },
            {
            "setTimeDelayUS",
                {
                    "Set the time-delay of the servo (µs).",
                        [this](const String &arg, AsyncWebSocketClient *client) -> bool {
                        unsigned long val;
                        return tryParse(arg, val) && this->servo.setTimeDelayUS(val);
                    }
                }
            },
            {
                "setTimeDelayMS",
                    {
                    "Set the time-delay of the servo (ms).",
                    [this](const String &arg, AsyncWebSocketClient *client)  -> bool {
                        unsigned long val;
                        return (tryParse(arg, val) && this->servo.setTimeDelayMS(val));
                    }
                }
            },
            {
                "setFrequency",
                    {
                    "Set the frequency of the PWM sent to servo.",
                    [this](const String &arg, AsyncWebSocketClient *client)  -> bool {
                        unsigned long val;
                        return tryParse(arg, val) && this->servo.setFrequency(uint32_t(val));
                    }
                }
            },
            {
                "setResolution",
                    {
                    "Set the resolution of the PWM signal sent to servo.",
                    [this](const String &arg, AsyncWebSocketClient *client) -> bool {
                        unsigned int val;
                        return (tryParse(arg, val) && this->servo.setResolution(uint8_t(val)));
                    }
                }
            },
            {
                "setMotion",
                    {
                    motionControl,
                    [this](const String &arg, AsyncWebSocketClient *client) -> bool {
                        ESPServo::motion_mode_t motionMode = ESPServo::stringToMotion(arg);
                        if (motionMode != ESPServo::INVALID) {
                            this->servo.setMotion(motionMode);
                            return true;
                        } else {
                            return false;
                        }
                    }
                }
            },
            {
                "setStartAngle",
                    {
                    "Set the starting angle for servo motion.",
                        [this](const String &arg, AsyncWebSocketClient *client) -> bool {
                            int val;
                            return (tryParse(arg, val) && this->servo.setStartAngle(val));
                        }
                    }
            },
            // TODO: These controls:
            {
                "setStopAngle",
                {
                    "Set the stopping angle for servo motion.",
                        [this](const String &arg, AsyncWebSocketClient *client) -> bool {
                            unsigned int val;
                            return tryParse(arg, val) && this->servo.setStopAngle(int(val));
                        }
                    }

            },
            {
                "setMinPWM",
                {
                    "Set the minimum angle for PWM control.",
                        [this](const String &arg, AsyncWebSocketClient *client) -> bool {
                            unsigned int val;
                            return tryParse(arg, val) && this->servo.setMinPWM(int(val));
                        }
                }
            },
            {
                "setMaxPWM",
                {
                    "Set the maximum angle for PWM control.",
                        [this](const String &arg, AsyncWebSocketClient *client) -> bool {
                            unsigned long val;
                            return tryParse(arg, val) && this->servo.setMaxPWM(val);
                        }
                }
            },
            {
                "setPin",
                {
                    "Set the pin which servo's connected to.",
                        [this](const String &arg, AsyncWebSocketClient *client) -> bool {
                        if (isDigit(arg[0])) {
                            unsigned int val;
                            if (tryParse(arg, val)) {
                                return this->servo.setPin(uint8_t(val));
                            } else {
                                return false;
                            }
                        } else {
                            return this->servo.setPin(arg);
                        }
                    }
                }
            },
            {
                "start",
                {
                    "Starts actuation of motor.",
                    [this](const String &, AsyncWebSocketClient *client)  -> bool{
                        this->servo.start();
                        return true;
                    }
                }
            },
            {
                "stop",
                {
                    "Stop servo actuation.",
                    [this](const String &, AsyncWebSocketClient *client)  -> bool{
                        this->servo.stop();
                        return true;
                    }
                }
            },
            {
                "setAngle",
                    {
                    "Set angle of servo motor without using delayed positioning.",
                    [this](const String &arg, AsyncWebSocketClient *client) -> bool {
                        unsigned int val;
                        return tryParse(arg, val) && this->servo.setMaxPWM(val);
                    }
                }
            },
            {
                    "help",
                    {
                            "Print these commands.",
                            [this](const String &, AsyncWebSocketClient *client) -> bool {
                                this->onPrintCommands(client);
                                return true;
                            }
                    }
            }
//            },
//            {
//                "getConfig",
//                    {
//                    "Get current configuration of servo motor.",
//                    [this](const String &, AsyncWebSocketClient *client) -> bool {
//                        client->text(getConfigDoc());
//                        return true;
//                    }
//                }
//            }
    };
}
const String& CommandProcessor::getConfigDoc() {
    responseDoc.clear();
    responseString.clear();
    responseDoc["device"] = "SERVO";
    responseDoc["request_made"] = "CURRENT_CONFIGURATION";
    responseDoc["response"]["pin"] = servo.getPin();
    responseDoc["response"]["angleStep"] = servo.getAngleStep();
    responseDoc["response"]["timeDelayUS"] = servo.getTimeDelayUS();
    responseDoc["response"]["position"] = servo.getAngle();
    responseDoc["response"]["frequency"] = servo.getFrequency();
    responseDoc["response"]["resolution"] = servo.getResolution();
    responseDoc["response"]["startAngle"] = servo.getStartAngle();
    responseDoc["response"]["stopAngle"] = servo.getStopAngle();
    responseDoc["response"]["maxAngle"] = servo.getMaxAngle();
    responseDoc["response"]["minPWM"] = servo.getMinPWM();
    responseDoc["response"]["maxPWM"] = servo.getMaxPWM();
    responseDoc["response"]["motion"] = servo.getMotionStr();
    responseDoc["response"]["motionStatus"] = servo.isActive() ? "Active" : "Inactive";
    serializeJsonPretty(responseDoc, responseString);
    return responseString;
}
void CommandProcessor::printConfigSerial() {
    Serial.println();
    printFixedln("----------- Current Configuration -----------");
    printFixed("Pin:");
    printFixedln(servo.getPin());
    printFixed("Angle-step:");
    printFixedln(servo.getAngleStep());
    printFixed("Time-delay (µs):");
    printFixedln(servo.getTimeDelayUS());
    printFixed("Position:");
    printFixedln(servo.getAngle());
    printFixed("Frequency:");
    printFixedln(servo.getFrequency());
    printFixed("Resolution:");
    printFixedln(servo.getResolution());
    printFixed("Start angle:");
    printFixedln(servo.getStartAngle());
    printFixed("Stop angle:");
    printFixedln(servo.getStopAngle());
    printFixed("Max angle:");
    printFixedln(servo.getMaxAngle());
    printFixed("Min PWM:");
    printFixedln(servo.getMinPWM());
    printFixed("Max PWM:");
    printFixedln(servo.getMaxPWM());
    printFixed("Motion mode:");
    printFixedln(servo.getMotionStr());
    printFixed("Motion state: ");
    printFixedln(servo.isActive() ? "Active" : "Inactive");

}
bool CommandProcessor::parseAndExecuteSerial(const CommandInput &input) {
    String attemptedName = input.raw.substring(0, input.raw.indexOf(' '));
    attemptedName.trim();
    String attemptedArg = input.raw.substring(input.raw.indexOf(' ') + 1);
    attemptedArg.trim();
    auto it = commands.find(attemptedName);
    if (it != commands.end()) {
        if (it->second.callback(attemptedArg, nullptr)) {
            sendOK(nullptr, it->first, attemptedArg);
            return true;
        } else {
            sendFailed(nullptr, it->first, attemptedArg);
            return false;
        }
    } else {
        Serial.println("Invalid command '" + input.raw + "'.");
        return false;
    }
}

JsonVariantConst CommandProcessor::findKey(ArduinoJson::JsonObjectConst obj, const String &key) {
    JsonVariantConst foundObj = obj[key];
    if (!foundObj.isNull()) {
        return foundObj;
    }
    for (JsonPairConst pair : obj) {
        JsonVariantConst nested = findKey(pair.value(), key);
        if (!nested.isNull()) {
            return nested;
        }
        return {};
    }
}


bool CommandProcessor::parseAndExecuteSocket(const CommandInput &input) {
    PRINT_UPDATELN("Request to parse and execute command received from WebSocket.");
    JsonDocument received;
    DeserializationError error = deserializeJson(received, input.raw);
    if (error) {
        PRINTF_UPDATELN("%s", input.raw.c_str());
        return true;
    } else {
        if (received["device"] == "SERVO") {
            JsonObject requestObj = received["request"].as<JsonObject>();
            PRINT_UPDATELN("Input contained key 'device' with value 'SERVO'. Continuing to parse.");
            bool allSuccess = true;
            for (JsonPair commandPair : requestObj) {
                const String &commandName = commandPair.key().c_str();
                PRINTF_UPDATELN("Extracted command name: %s", commandName.c_str());
                const String arg = commandPair.value().as<String>();
                PRINTF_UPDATELN("Extracted command argument: %s", arg.c_str());
                auto it = commands.find(commandName);
                if (it != commands.end()) {
                    PRINTF_UPDATELN("Found command '%s'. Validating command argument...", it->first.c_str());
                    bool success = it->second.callback(arg, input.client);
                    if (success) {
                        PRINTF_UPDATELN("Successfully validated command '%s' with argument '%s'.", it->first.c_str(), arg.c_str());
                        sendOK(input.client, commandName, arg);
                    } else {
                        PRINTF_UPDATELN("Failed to validate command '%s' with argument '%s'.", it->first.c_str(), arg.c_str());
                        sendFailed(input.client, String(commandPair.key().c_str()), commandPair.value().as<String>());
                        allSuccess = false;
                    }
                } else {
                    sendFailed(input.client, String(commandPair.key().c_str()), commandPair.value().as<String>());
                    allSuccess = false;
                }
            }
            return allSuccess;
        }
        PRINTF_UPDATELN("Unknown device. String received: %s", input.raw.c_str());
        return false;
    }
}

bool CommandProcessor::handle(const CommandInput &input) {
    if (input.client != nullptr) {
        return parseAndExecuteSocket(input);
    } else {
        return parseAndExecuteSerial(input);
    }
}