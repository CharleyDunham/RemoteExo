// ServoController.h
#pragma once
#include <Arduino.h>
#include <functional>
#include <esp_timer.h>

using AngleCallback = std::function<void(int)>;

class ServoController {
public:
    enum Mode { ONE_SHOT_CW, ONE_SHOT_CCW, ONE_WAY_CW, ONE_WAY_CCW,
                BIDIRECTIONAL, FALLBACK, INVALID };

    explicit ServoController(uint8_t pin, int maxAngle=270);

    [[nodiscard]] Mode getCurrentMode() const { return mode_; }

    void setup();
    void loop();  // call this in Arduino::loop()

    void setMode(Mode);
    void setPosition(unsigned int);
    [[nodiscard]] unsigned int getPosition() const;

    void setTimeDelayUS(unsigned long);
    [[nodiscard]] unsigned long getTimeDelayUS() const;

    void setMinPWM(unsigned long);
    [[nodiscard]] unsigned long getMinPWM() const;

    void setMaxPWM(unsigned long);
    [[nodiscard]] unsigned long getMaxPWM() const;

    void setAngleStep(int);
    [[nodiscard]] int getAngleStep() const;

    void setStartAngle(int);
    [[nodiscard]] int getStartAngle() const {return startAngle_; }

    void setStopAngle(int);
    [[nodiscard]] int getStopAngle() const {return stopAngle_; }


    [[nodiscard]] uint8_t getPin() const { return pin_; }
    void setPin(uint8_t pin);

    void start();
    void stop();

    void onAngle(AngleCallback cb) { cb_ = cb; }

    static Mode getModeName(const char* mode) {
        if (strcmp(mode, "ONE_SHOT_CW") == 0) {
            return ONE_SHOT_CW;
        }
        if (strcmp(mode, "ONE_SHOT_CCW") == 0) {
            return ONE_SHOT_CCW;
        }
        if (strcmp(mode, "ONE_WAY_CW") == 0) {
            return ONE_WAY_CW;
        }
        if (strcmp(mode, "ONE_WAY_CCW") == 0) {
            return ONE_WAY_CCW;
        }
        if (strcmp(mode, "BIDIRECTIONAL") == 0) {
            return BIDIRECTIONAL;
        }
        if (strcmp(mode, "FALLBACK") == 0) {
            return FALLBACK;
        }
        return INVALID;
    }
    static const char *modeStr(Mode mode) {
        switch (mode) {
            case ONE_SHOT_CW:
                return "ONE_SHOT_CW";
            case ONE_SHOT_CCW:
                return "ONE_SHOT_CCW";
            case ONE_WAY_CW:
                return "ONE_WAY_CW";
            case ONE_WAY_CCW:
                return "ONE_WAY_CCW";
            case BIDIRECTIONAL:
                return "BIDIRECTIONAL";
            case FALLBACK:
                return "FALLBACK";
            default:
                return "INVALID";
        }
    }
    [[nodiscard]] bool listener() const {
        return (cb_ != nullptr);
    }
private:
    uint8_t          pin_, channel_;
    Mode             mode_, prevMode_;
    int              pos_, step_, maxAngle_, startAngle_, stopAngle_;
    unsigned long    delayUs_, minPWM_  = 500, maxPWM_ = 2500, lastTime = 0, FALLBACK_DELAY = 3000;
    volatile bool    tick_;
    esp_timer_handle_t timer_, fallbackTimer_;
    AngleCallback    cb_;
    void startFallback() const;
    void updateDuty() const;
    static void IRAM_ATTR timerFn(void* arg);
    static void IRAM_ATTR timerFallback(void *arg);
    const esp_timer_create_args_t fallbackArgs = {
        .callback = timerFallback,
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "fallbackTimer",
        .skip_unhandled_events = false
    };
};
