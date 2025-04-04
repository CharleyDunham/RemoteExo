#ifndef UNTITLED_COMMAND_H
#define UNTITLED_COMMAND_H
#include "Servo.h"
#include <AsyncWebSocket.h>
#include <map>
#include <Arduino.h>
#include <ArduinoJson.h>
#define sfout(...) Serial.printf(__VA_ARGS__);
#define sfoutln(...) do { Serial.printf(__VA_ARGS__); Serial.println(); } while (0)
#define sout(...) Serial.println(__VA_ARGS__);
#define soutln(...) do { Serial.printf(__VA_ARGS__); Serial.println(); } while (0)

#define PRINTF_UPDATELN(...) if (printUpdates) sfoutln(__VA_ARGS__)
#define PRINTF_UPDATE(...) if (printUpdates) sfout(__VA_ARGS__)
#define PRINT_UPDATELN(...) if (printUpdates) soutln(__VA_ARGS__)
#define PRINT_UPDATE(...) if (printUpdates) sout(__VA_ARGS__)

class CommandProcessor {
public:
    bool printUpdates = true;
    typedef struct {
        String raw;
        AsyncWebSocketClient *client;
    } CommandInput;
    const String& getConfigDoc();
    void printConfigSerial();

    explicit CommandProcessor(ESPServo &servo);
    bool handle(const CommandInput& input);
    //void handle(const CommandInput &input);
private:

    typedef enum {
        SET_ANGLE_STEP, /*-------------------------------| Command type to set the angle-step (º / timeDelay µ). */
        SET_ANGLE, /*------------------------------------| Write an angle to the servo. Ignores the current motion. */
        SET_TIME_DELAY, /*-------------------------------| Set the time-delay of servo. Delay is used to increment the
                                                        angle angleStepº per timed callback invocation [µs]. */
        SET_TIME_DELAY_MS, /*----------------------------| Set the timed invocation [ms]. */
        SET_FREQUENCY,
        SET_RESOLUTION,
        SET_MOTION, /*-----------------------------------| Set the motion type for servo actuation. */
        SET_START_ANGLE, /*------------------------------| Set the starting angle for the motion. */
        SET_STOP_ANGLE, /*-------------------------------| Set the stopping angle for the motion. */
        SET_MAX_ANGLE, /*--------------------------------| Set the maximum angle. This is used to calculate the PWM signal.*/
        SET_MIN_PWM, /*----------------------------------| Set the minimum PW signal (µs), used to scale PWM signal. */
        SET_MAX_PWM, /*----------------------------------| Set the maximum PW signal (µs), also used to scale PWM signal. */
        SET_PIN, /*--------------------------------------| Set/change the pin servo's attached to. */
        START, /*----------------------------------------| Start the servo actuation at the defined speed. */
        STOP, /*-----------------------------------------| Stop servo actuation. */
        HELP, /*-----------------------------------------| Print all the commands. */
        GET_CONFIG, /*-----------------------------------| Get the servo's current configuration. */
        NOT_FOUND /*-------------------------------------| Used for invalid commands. */
    } command_type_t;

    static JsonVariantConst findKey(JsonObjectConst obj, const String &key);
    struct CommandEntry {
        String commandDescription;
        std::function<bool(const String&, AsyncWebSocketClient*)> callback;
    } command_t;

    std::map<String, CommandEntry> commands;

    static bool tryParse(const String& args, long &value);
    static bool tryParse(const String &args, int &value);
    static bool tryParse(const String &args, unsigned long &value);
    static bool tryParse(const String &args, unsigned int &val);
    template <typename T>
    void sendOK(AsyncWebSocketClient *client, const String &command, const T &val);

    void sendOK(AsyncWebSocketClient *client, const String &command, const String &args);

    template <typename T>
    void sendFailed(AsyncWebSocketClient *client, const String &command, const T &valueAttempted);

    void sendFailed(AsyncWebSocketClient *client, const String &command, const String &argsAttempted);
    void sendFailed(const CommandInput &input);
    //void parseAndExecuteSerial(const CommandInput &input);
    bool parseAndExecuteSerial(const CommandInput &input);
    bool parseAndExecuteSocket(const CommandInput &input);
    ESPServo &servo;
    JsonDocument responseDoc;
    String responseString;
    template <typename T>
    static void printFixed(T &args) {
        Serial.printf("%-20s", String(args).c_str());
    }
    template <typename T>
    static void printFixedln(const T &args) {
        Serial.printf("%-20s\n", String(args).c_str());
    }
    static void printFixedln(const String &args) {
        Serial.printf("%-20s\n", args.c_str());
    }

    static void printFixed(const String &args) {
        Serial.printf("%-20s", args.c_str());
    }

    void onPrintCommands(AsyncWebSocketClient *client);
    const String motionControl = "Control how servo actuates. Valid entries: "
                                 "\n\tONE_SHOT_CW  \tMove clockwise once to stopAngle, stopping when reached."
                                 "\n\tONE_SHOT_CCW \tMove counterclockwise once to startAngle, stopping when reached."
                                 "\n\tONE_WAY_CW   \tMove clockwise up until reaching stopAngle, then move back to start angle, continuing loop."
                                 "\n\tONE_WAY_CCW  \tMove counterclockwise back until reaching startAngle, then move back to stopAngle, continuing loop."
                                 "\n\tBIDIRECTIONAL\tMove clockwise and counterclockwise with the given angleStep and timeDelay.";


};

#endif //UNTITLED_COMMAND_H
