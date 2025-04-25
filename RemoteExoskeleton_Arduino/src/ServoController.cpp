#include "ServoController.h"


uint8_t ServoController::channelCount = 0;
void ServoController::timerCB(void *arg) {
    auto instance = static_cast<ServoController*>(arg);
    instance->tick_ = true;
}
void ServoController::fallbackTimerCB(void *arg) {
    auto instance = static_cast<ServoController*>(arg);
    if (!esp_timer_is_active(instance->timer_)) {
        esp_err_t err = esp_timer_start_periodic(instance->timer_, instance->delayUs_);
        if (err != ESP_OK) {
            Serial << "Failed to restart servo timer." << endl;
        }
    }
}
ServoController::ServoController(uint8_t pin, unsigned int maxAngle) :
    pin_(pin),
    maxAngle_(maxAngle),
    timer_(nullptr),
    fallbackTimer_(nullptr),
    pos_(0),
    delayUs_(100000),
    pwmMin_(500),
    pwmMax_(2500),
    startAngle_(0),
    stopAngle_(270),
    angleStep_(1),
    fallbackTimerArgs_({
        .callback = fallbackTimerCB,
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "fallback",
        .skip_unhandled_events = false
    }),
    timerArgs_({
        .callback = timerCB,
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "servo",
        .skip_unhandled_events = false
    }),
    tick_(false), angleNotify_(nullptr)
{
    channelCount++;
}


ServoController::~ServoController() {
    esp_timer_stop(timer_);
    esp_timer_stop(fallbackTimer_);
    esp_timer_delete(timer_);
    esp_timer_delete(fallbackTimer_);
}
void ServoController::setup() {
    ledcSetup(channelCount, 50, 10);
    ledcAttachPin(pin_, channelCount);
    updateDuty();
    tick_ = false;
    const auto error = esp_timer_create(&timerArgs_, &timer_);
    if (error != ESP_OK) {
        Serial << "Failed to create timer: " << error << endl << "Throwing std::runtime_error." << endl;
        throw std::runtime_error("Failed to create timer");
    }
    const auto error2 = esp_timer_create(&fallbackTimerArgs_, &fallbackTimer_);
    if (error2 != ESP_OK) {
        Serial << "Failed to create fallback timer: " << error2 << endl << "Throwing std::runtime_error." << endl;
        throw std::runtime_error("Failed to create fallback timer");
    }
}
void ServoController::loop() {
    if (!tick_) return;
    tick_ = false;
    if (angleStep_ == 0) disableMotion();
    switch (motion_) {
        case LOOP: {
            /* Determine the intended direction from the angle-step. */
            if (angleStep_ < 0) {
                if (pos_ + angleStep_ < stopAngle_) {
                    pos_ = startAngle_;
                } else {
                    // keep decrementing
                    pos_ += angleStep_;
                }
            } else if (angleStep_ > 0) {
                // check > stopAngle_
                if (pos_ + angleStep_ > stopAngle_) {
                    // go back to start
                    pos_ = startAngle_;
                } else {
                    // keep incrementing
                    pos_ += angleStep_;
                }
            }
        } break;
        case SWEEP: {
            if (angleStep_ < 0) {
                /* Currently decrementing. */
                if (pos_ + angleStep_ > startAngle_) {
                    /* Continue decrement. */
                    pos_ += angleStep_;
                } else {
                    /* Set position to 0. */
                    pos_ = 0;
                    /* Invert angle step. */
                    angleStep_ *= -1;
                }
            } else {
                if (pos_ + angleStep_ < stopAngle_) {
                    /* Continue incrementing. */
                    pos_ += angleStep_;
                } else {
                    /* Set position to the max angle. */
                    pos_ = maxAngle_;
                    /* Invert angle-step. */
                    angleStep_ *= -1;
                }
            }
        } break;
        case ONE_SHOT: {
            if (angleStep_ < 0) {
                if (pos_ + angleStep_ >= startAngle_) {
                    pos_ += angleStep_;
                } else {
                    disableMotion();
                }
            } else {
                if (pos_ + angleStep_ <= stopAngle_) {
                    pos_ += angleStep_;
                } else {
                    disableMotion();
                }
            }
        } break;
        default: disableMotion(); break;
    }
    updateDuty();
    if (angleNotify_) angleNotify_(pos_);
}
void ServoController::setMaxPWM(unsigned long m) {
    if (m <= pwmMin_) {
        Serial << "new max PWM value cannot be <= existing PWM value." << endl;
        return;
    }
    const bool wasRunning = esp_timer_is_active(timer_);
    if (wasRunning) disableMotion();
    pwmMax_ = m;
    Serial << "new max PWM value: " << pwmMax_ << endl;
    if (wasRunning) disableMotion();
}
void ServoController::setMinPWM(unsigned long m) {
    if (m >= pwmMax_) {
        Serial << "new min PWM value cannot be >= existing PWM value." << endl;
        return;
    }
    const bool wasRunning = esp_timer_is_active(timer_);
    if (wasRunning) disableMotion();
    pwmMin_ = m;
    Serial << "new min PWM value: " << pwmMin_ << endl;
    if (wasRunning) disableMotion();
}

void ServoController::setAngleStep(int s) {
    if (abs(s) > maxAngle_) {
        Serial << "angle-step size cannot exceed the maximum range of servo." << endl;
        return;
    }
    bool wasRunning = esp_timer_is_active(timer_);
    if (wasRunning) disableMotion();
    angleStep_ = s;
    Serial << "new angle-step: " << angleStep_ << endl;
    if (wasRunning) enableMotion();
}
void ServoController::setPosition(unsigned int a) {
    if (a > maxAngle_) {
        Serial << "New position '" << a << "' exceeds maximum range '" << maxAngle_ << "'." << endl;
    }
    bool running = esp_timer_is_active(timer_);
    if (running) disableMotion();
    pos_ = a;
    updateDuty();
    if (running) enableMotion();
    if (angleNotify_) angleNotify_(pos_);
}
void ServoController::setMotion(Motion m) {
    if (m == OFF) {
        Serial << "Disabling servo..." << endl;
    }
    if (esp_timer_is_active(timer_)) {
        auto err = esp_timer_stop(timer_);
        if (err != ESP_OK) Serial << "Stopping servo timer failed: " << err << endl;
    }
}
void ServoController::setTimeDelay(unsigned long t) {
    if (t < pwmMin_) {
        Serial << "new time delay cannot be < minimum PWM value." << endl;
        return;
    }
    bool wasRunning = esp_timer_is_active(timer_);
    if (wasRunning) disableMotion();
    delayUs_ = t;
    Serial << "new time delay: " << delayUs_ << endl;
    if (wasRunning) enableMotion();
}
void ServoController::setStartAngle(unsigned int a ) {
    if (startAngle_ < 0) {
        Serial << "new start angle cannot be < 0." << endl;
        return;
    }
    if (a > maxAngle_) {
        Serial << "new start angle cannot be > maximum range." << endl;
        return;
    }
    bool wasRunning = esp_timer_is_active(timer_);
    if (wasRunning) disableMotion();
    startAngle_ = a;
    Serial << "new start angle: " << startAngle_ << endl;
    if (wasRunning) enableMotion();
}
void ServoController::setStopAngle(unsigned int a) {
    if (stopAngle_ < 0) {
        Serial << "new stop angle cannot be < 0." << endl;
        return;
    }
    if (a > maxAngle_) {
        Serial << "new stop angle cannot be > maximum range." << endl;
        return;
    }
    bool wasRunning = esp_timer_is_active(timer_);
    if (wasRunning) disableMotion();
    stopAngle_ = a;
    Serial << "new stop angle: " << stopAngle_ << endl;
    if (wasRunning) enableMotion();
}
void ServoController::updateDuty() {
    const auto pulse = map(pos_, 0, maxAngle_, pwmMin_, pwmMax_);
    constexpr uint32_t PERIOD_US = 20000;
    constexpr uint32_t MAX_TICKS = (1u << 10) - 1;
    uint32_t duty = (pulse * MAX_TICKS) / PERIOD_US;
    ledcWrite(channelCount, duty);
}
void ServoController::setPin(uint8_t pin) {
    bool wasRunning = esp_timer_is_active(timer_);
    if (wasRunning) disableMotion();
    pin_ = pin;
    ledcAttachPin(pin_, channelCount);
    Serial << "new pin: " << pin_ << endl;
    if (wasRunning) enableMotion();
}
void ServoController::enableMotion() {
    if (esp_timer_is_active(timer_)) return;
    const auto error = esp_timer_start_periodic(timer_, delayUs_);
    if (error != ESP_OK) {
        Serial << "Failed to start servo timer." << endl;
        return;
    }
    Serial << "Servo enabled." << endl;
}
void ServoController::setMaxAngle(unsigned int a) {
    bool wasRunning = esp_timer_is_active(timer_);
    if (wasRunning) disableMotion();
    maxAngle_ = a;
    Serial << "new max angle: " << maxAngle_ << endl;
    if (wasRunning) enableMotion();
}
void ServoController::disableMotion() {
    if (!esp_timer_is_active(timer_)) return;
    const auto error = esp_timer_stop(timer_);
    if (error != ESP_OK) {
        Serial << "Failed to stop servo timer." << endl;
        return;
    }
    Serial << "Servo disabled." << endl;
}

