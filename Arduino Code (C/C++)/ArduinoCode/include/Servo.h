//
// Created by Sullivan Bryant on 3/31/25.
//

#ifndef UNTITLED_SERVO_H
#define UNTITLED_SERVO_H
//
// Created by Sullivan Bryant on 3/29/25.
//

#include <Arduino.h>
#include <esp_timer.h>
#include <esp_event.h>
#include <map>
#include <esp_err.h>

#define sfout(...) Serial.printf(__VA_ARGS__);
#define sfoutln(...) do { Serial.printf(__VA_ARGS__); Serial.println(); } while (0)
#define sout(...) Serial.println(__VA_ARGS__);
#define soutln(...) do { Serial.printf(__VA_ARGS__); Serial.println(); } while (0)

#define PRINTF_UPDATELN(...) if (printUpdates) sfoutln(__VA_ARGS__)
#define PRINTF_UPDATE(...) if (printUpdates) sfout(__VA_ARGS__)
#define PRINT_UPDATELN(...) if (printUpdates) soutln(__VA_ARGS__)
#define PRINT_UPDATE(...) if (printUpdates) sout(__VA_ARGS__)


class ESPServo {
public:
    typedef enum {
        ONE_SHOT_CW,
        ONE_SHOT_CCW,
        ONE_WAY_CW,
        ONE_WAY_CCW,
        BIDIRECTIONAL,
        FALLBACK,
        INVALID = -1
    } motion_mode_t;
    static const char* motionToString(const motion_mode_t &motion);
    static motion_mode_t stringToMotion(const String& input);
    static void onTimer(void *arg);
    esp_timer_cb_t timerCallback;
private:
    bool success;
    bool reconfigure(const uint8_t &newPin, const uint32_t &newFreq, const uint8_t &newRes);
    void onTimerInstance();
    motion_mode_t prevMotion;
    motion_mode_t currentMotion;
    int pos;
    int angleStep;
    long timeDelayUS;
    int maxAngle;
    int startAngle;
    int stopAngle;
    uint8_t pin;
    uint8_t resolution;
    uint8_t channel;
    bool printUpdates;
    long maxTicks;
    uint32_t frequency;
    unsigned long minPWM;
    unsigned long maxPWM;
    esp_timer_handle_t servoTimer;
    esp_timer_create_args_t servoArgs;
public:
    explicit ESPServo(motion_mode_t _motion = ONE_WAY_CW,
                      int _pos = 0,
                      int _angleStep = 1,
                      long _timeDelayUS = 50000,
                      int _maxAngle = 270,
                      int _startAngle = 0,
                      int _stopAngle = 270,
                      uint8_t _pin = D4,
                      uint32_t freq = 50,
                      uint8_t res = 10,
                      unsigned long _minPWM = 500,
                      unsigned long _maxPWM = 2500);

    void writeCurrentPosition() const;
    void start();
    bool stop(); // Returns if was stopped from being active.
    bool setTimeDelayUS(const unsigned long &newTimeDelayUS);
    long getTimeDelayUS() const { return timeDelayUS; }

    bool setTimeDelayMS(const unsigned long &newTimeDelayMS);
    long getTimeDelayMS() const { return timeDelayUS / 1000; }

    bool setAngleStep(const int& newAngleStep);
    int getAngleStep() const { return angleStep; }

    bool setPin(const uint8_t &newPin);
    bool setPin(const String &newPin);
    uint8_t getPin() const { return pin; }

    void setMotion(const motion_mode_t &motion);
    motion_mode_t getMotion() const { return currentMotion; }

    bool setStartAngle(const int &_startAngle);
    int getStartAngle() const;

    bool setStopAngle(const int &stopAngle);
    int getStopAngle() const { return stopAngle; }

    bool setMaxAngle(const int &newAngle);
    int getMaxAngle() const { return maxAngle; }

    bool setMinPWM(const int &newPWM);
    unsigned long getMinPWM() const {return minPWM; }
    bool setMaxPWM(const unsigned long &newPWM);
    unsigned long getMaxPWM() const { return maxPWM; }

    bool setAngle(const int &newAngle);
    int getAngle() const { return pos; }

    bool setFrequency(const uint32_t &newFrequency);
    uint32_t getFrequency() const { return frequency; }

    bool setResolution(const uint8_t &newRes);
    uint8_t getResolution() const { return resolution; }
    String getMotionStr() const { return ESPServo::motionToString(currentMotion); }

    bool isActive() const {
        return esp_timer_is_active(servoTimer);
    }
    bool attach(const uint8_t &_pin = D4);

    void showUpdates(bool show) {
        printUpdates = show;
    }

    long getMaxTicks() const {
        return maxTicks;
    }
    static const std::map<String, uint8_t> pinMap;
};


#endif //UNTITLED_SERVO_H
