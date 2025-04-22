// ServoController.cpp
#include "ServoController.h"
#include <Arduino.h>
#include <esp_timer.h>

//------------------------------------------------------------------------------
// Constructor (nothing hardware‐y yet)
ServoController::ServoController(const uint8_t pin, const int maxAngle)
    : pin_(pin),
      channel_(0),
      mode_(ONE_WAY_CW),
      pos_(0),
      step_(1),
      maxAngle_(maxAngle),
      delayUs_(20000),
      timer_(nullptr), fallbackTimer_(nullptr), tick_(false),
    startAngle_(0), stopAngle_(maxAngle), prevMode_(ONE_WAY_CW)
{
}

void ServoController::setStartAngle(int a) {
    if (a < 0 || a > maxAngle_)
        throw std::invalid_argument("invalid start angle");
    startAngle_ = a;
}

void ServoController::setStopAngle(int a) {
    if (a < 0 || a > maxAngle_)
        throw std::invalid_argument("invalid stop angle");
    stopAngle_ = a;
}

//------------------------------------------------------------------------------
// private helper: map `pos_` → LEDC duty
void ServoController::updateDuty() const {
    // 1) map [0…maxAngle_] → [minPWM…maxPWM]
    const unsigned long pulse = map(pos_, 0, maxAngle_, minPWM_, maxPWM_);
    // 2) convert to duty counts (we assume 50 Hz, i.e. period = 20,000 µs)
    //    and a fixed 10‑bit resolution (0…1023).
    constexpr unsigned long period = 1000000UL / 50;
    constexpr uint32_t maxTicks = (1 << 10) - 1;
    const uint32_t duty = (pulse * maxTicks) / period;
    ledcWrite(channel_, duty);
}

//------------------------------------------------------------------------------
// call once from setup()
void ServoController::setup() {
    // 1) configure hardware PWM
    constexpr uint8_t resBits = 10;

    uint32_t freqHz = ledcSetup(channel_, 50, resBits);
    ledcAttachPin(pin_, channel_);

    // initialize to current pos
    updateDuty();

    // 2) create the ESP periodic timer
    esp_timer_create_args_t args = {
        .callback = &ServoController::timerFn,
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "servoTick",
        .skip_unhandled_events = false
    };
    if (esp_timer_create(&args, &timer_) != ESP_OK) {
        throw std::runtime_error("Failed to create servo timer");
    }

    // 3) Fallback timer esp
    if (esp_timer_create(&fallbackArgs, &fallbackTimer_) != ESP_OK) {
        throw std::runtime_error("Failed to init fallback timer.");
    }
}

//------------------------------------------------------------------------------
// change how often `loop()` will see `tick_==true`
void ServoController::setTimeDelayUS(unsigned long t) {
    if (t < minPWM_) throw std::invalid_argument("delay < minPWM");
    bool wasRunning = false;
    try { wasRunning = esp_timer_is_active(timer_); } catch (...) {
    }
    if (wasRunning) esp_timer_stop(timer_);
    delayUs_ = t;
    if (wasRunning) esp_timer_start_periodic(timer_, delayUs_);
}

unsigned long ServoController::getTimeDelayUS() const {
    return delayUs_;
}

//------------------------------------------------------------------------------
// min/max PWM bounds
void ServoController::setMinPWM(unsigned long m) {
    if (m >= maxPWM_) throw std::invalid_argument("minPWM >= maxPWM");
    minPWM_ = m;
}

unsigned long ServoController::getMinPWM() const { return minPWM_; }

void ServoController::setMaxPWM(unsigned long m) {
    if (m <= minPWM_) throw std::invalid_argument("maxPWM <= minPWM");
    maxPWM_ = m;
}

unsigned long ServoController::getMaxPWM() const { return maxPWM_; }

//------------------------------------------------------------------------------
// angle step
void ServoController::setAngleStep(int s) {
    if (s == 0 || abs(s) > maxAngle_)
        throw std::invalid_argument("invalid step");
    step_ = s;
}

int ServoController::getAngleStep() const { return step_; }

//------------------------------------------------------------------------------
// set/get absolute position
void ServoController::setPosition(unsigned int a) {
    if (a > static_cast<unsigned>(maxAngle_))
        throw std::invalid_argument("angle > maxAngle");
    pos_ = a;
    updateDuty();
    if (cb_) cb_(pos_);
}

unsigned int ServoController::getPosition() const {
    return pos_;
}

//------------------------------------------------------------------------------
// change motion mode
void ServoController::setMode(Mode m) {
    if (m == INVALID)
        throw std::invalid_argument("cannot set to fallback/invalid");
    bool wasRunning = false;
    try { wasRunning = esp_timer_is_active(timer_); } catch (...) {
    }
    if (wasRunning) esp_timer_stop(timer_);
    if (m == FALLBACK) { lastTime = millis(); prevMode_ = mode_;}
    mode_ = m;
    if (wasRunning) esp_timer_start_periodic(timer_, delayUs_);
}

//------------------------------------------------------------------------------

void ServoController::start() {
    if (!timer_) {
        setup();
    }
    if (auto response = esp_timer_start_periodic(timer_, delayUs_); response == ESP_ERR_INVALID_ARG) {
        throw std::invalid_argument("failed to start timer");
    } else if (response == ESP_ERR_INVALID_STATE) {
        esp_timer_stop(timer_);
        esp_timer_start_periodic(timer_, delayUs_);
    }

}

void ServoController::stop() {
    if (!timer_) {
        setup();
    }
    esp_timer_stop(timer_);
}


//------------------------------------------------------------------------------
// call this from Arduino::loop()
void ServoController::loop() {
    if (!tick_) return;
    tick_ = false;

    switch (mode_) {
         case ONE_SHOT_CW:
             /* Make sure angle-step is +. */
             if (step_ < 0) step_ *= -1; /* Increment clockwise. */
             if (pos_ + step_ <= stopAngle_) {
                 pos_ += step_; /* Continue incrementing, setting new position. */
             } else {
                 stop(); /* Stop timer. */
             }
             break;
         case ONE_SHOT_CCW: /* One-shot motion counterclockwise. Moves at constant speed, starts at stopAngle, stops at startAngle.  */
             /* Make sure angle-step is -. */
             if (step_ > 0) step_ *= -1;
             /* Increment ccw. */
             if (pos_ + step_ >= startAngle_) {
                 /* Continue incrementing, setting new position. */
                 pos_ += step_;
             } else {
                 /* Stop timer. */
                 stop();
             }
             break;
         case ONE_WAY_CW:
             /* Make sure angle-step is +. */
             if (step_ < 0) step_ *= -1;
             /* Increment CW. */
             if (pos_ + step_ <= stopAngle_) {
                 pos_ += step_;
             } else {
                 /* Back to 0º. */
                 pos_ = 0;
                 prevMode_ = ONE_WAY_CW;
                 setMode(FALLBACK);
             }
             break;
         case ONE_WAY_CCW:
             /* Make sure angle-step is -. */
             if (step_ > 0) step_ *= -1;
             /* Increment CCW. */
             if (pos_ + maxAngle_ >= startAngle_) {
                 pos_ += step_;
             } else {
                 /* Back to max angle.*/
                 pos_ = maxAngle_;
                 prevMode_ = ONE_WAY_CCW;
                 setMode(FALLBACK);
             }
             break;
         case FALLBACK: {
             // TODO: see if this works.
             startFallback();
             stop();
             return;
         }
         case INVALID: {
             stop();
             Serial.println("Current motion is set INVALID.");
         } break;
         default: {
             /* Bidirectional */
             /* Check the current direction of servo. */
             if (step_ < 0) {
                 /* Currently decrementing. */
                 if (pos_ + step_ > startAngle_) {
                     /* Continue decrement. */
                     pos_ += step_;
                 } else {
                     /* Set position to 0. */
                     pos_ = 0;
                     /* Invert angle step. */
                     step_ *= -1;
                 }
             } else {
                 if (pos_ + step_ < stopAngle_) {
                     /* Continue incrementing. */
                     pos_ += step_;
                 } else {
                     /* Set position to the max angle. */
                     pos_ = maxAngle_;
                     /* Invert angle-step. */
                     step_ *= -1;
                 }
             }
         } break;
     }

    // update the PWM and notify
    updateDuty();
    if (cb_) cb_(pos_);
}


void ServoController::setPin(const uint8_t pin) {
    stop();
    pin_ = pin;
    channel_ = digitalPinToAnalogChannel(pin_);
}

void ServoController::startFallback() const {
    if (const auto result = esp_timer_start_once(fallbackTimer_, FALLBACK_DELAY * 1000); result != ESP_OK) {
        esp_timer_stop(fallbackTimer_);
        esp_timer_start_once(fallbackTimer_, FALLBACK_DELAY * 1000);
    }
}

//------------------------------------------------------------------------------
// static timer callback—just flip the flag
void IRAM_ATTR ServoController::timerFn(void *arg) {
    static_cast<ServoController *>(arg)->tick_ = true;
}
// i have to stop the servo timer so that each loop invocation doesn't start fallback timer.
void IRAM_ATTR ServoController::timerFallback(void *arg) {
    auto instance = static_cast<ServoController *>(arg);
    instance->mode_ = instance->prevMode_;
    instance->prevMode_ = FALLBACK;
    esp_timer_start_periodic(instance->timer_, instance->delayUs_);
}



