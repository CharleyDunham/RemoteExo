#ifndef SERVOCONTROLLER_H
#define SERVOCONTROLLER_H

#include <Arduino.h>
#include <functional>
#include <esp_timer.h>
#include <esp_event.h>
#include "SerialStream.h"

class ServoController {
public:
    enum Motion {
        LOOP,
        SWEEP,
        ONE_SHOT,
        OFF
    };
    static const char *motionString(Motion motion) {
        switch (motion) {
            case LOOP: return "LOOP";
            case SWEEP: return "SWEEP";
            case ONE_SHOT: return "ONE_SHOT";
            default: return "OFF";
        }
    }
    static Motion fromString(const char *motion) {
        if (strcmp(motion, "LOOP") == 0) return LOOP;
        if (strcmp(motion, "SWEEP") == 0) return SWEEP;
        if (strcmp(motion, "ONE_SHOT") == 0) return ONE_SHOT;
        return OFF;
    }
    explicit ServoController(uint8_t pin = D4, unsigned int maxAngle = 270);
    ~ServoController();
    void setup();
    void loop();

    void setPin(uint8_t pin);
    [[nodiscard]] uint8_t getPin() const { return pin_; }


    void setMaxAngle(unsigned int maxAngle);
    [[nodiscard]] unsigned int getMaxAngle() const { return maxAngle_; }

    void setMotion(Motion motion);
    [[nodiscard]] Motion getMotion() const { return motion_; }

    void setTimeDelay(unsigned long delayUs);
    [[nodiscard]] unsigned long getTimeDelay() const { return delayUs_; }

    void setMinPWM(unsigned long pwmMin);
    [[nodiscard]] unsigned long getPwmMin() const { return pwmMin_; }

    void setMaxPWM(unsigned long pwmMax);
    [[nodiscard]] unsigned long getPwmMax() const { return pwmMax_; }

    void setStartAngle(unsigned int startAngle);
    [[nodiscard]] unsigned int getStartAngle() const { return startAngle_; }

    void setStopAngle(unsigned int stopAngle);
    [[nodiscard]] unsigned int getStopAngle() const { return stopAngle_; }

    void setAngleStep(int angleStep);
    [[nodiscard]] int getAngleStep() const { return angleStep_; }

    void setPosition(unsigned int pos);
    [[nodiscard]] unsigned int getPosition() const { return pos_; }

    [[nodiscard]] bool isActive() const { return esp_timer_is_active(timer_); }

    void enableMotion();
    void disableMotion();
    using callback = std::function<void(int angle)>;
    void addAngleNotify(callback cb) { angleNotify_ = std::move(cb); }
private:
    std::function<void(int angle)> angleNotify_;
    void updateDuty();
    volatile bool tick_;
    uint8_t pin_;
    unsigned int maxAngle_;
    Motion motion_;
    unsigned int pos_;
    unsigned long delayUs_;
    unsigned long pwmMin_;
    unsigned long pwmMax_;
    unsigned int startAngle_;
    unsigned int stopAngle_;
    int angleStep_;
    esp_timer_handle_t timer_;
    esp_timer_create_args_t timerArgs_;
    esp_timer_handle_t fallbackTimer_;
    esp_timer_create_args_t fallbackTimerArgs_;
    static void IRAM_ATTR timerCB(void *arg);
    static void IRAM_ATTR fallbackTimerCB(void *arg);
    static uint8_t channelCount;
};
#endif //SERVOCONTROLLER_H
