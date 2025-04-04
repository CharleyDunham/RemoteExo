//
// Created by Sullivan Bryant on 3/30/25.
//
#include "Servo.h"


void ESPServo::onTimer(void *arg) {
    auto instance = static_cast<ESPServo *>(arg);
    instance->onTimerInstance();
}


const char* ESPServo::motionToString(const motion_mode_t &motion) {
    switch (motion) {
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

ESPServo::motion_mode_t ESPServo::stringToMotion(const String &input) {
    if (input.startsWith("ONE_")) { /* Fairly efficient way of processing. First, check if it starts with 'ONE_'. */
        if (input.startsWith("ONE_SHOT_")) { /* Then, check if it's a one-shot.  */
            if (input.equals("ONE_SHOT_CW")) {
                return ONE_SHOT_CW;
            } else if (input.equals("ONE_SHOT_CCW")) {
                return ONE_SHOT_CCW;
            } else {    /* Invalid if it starts with ONE_SHOT but doesn't end with _CW or _CCW. */
                return INVALID;
            }
        } else if (input.startsWith("ONE_WAY_")) {   /* Instead of ONE_SHOT_, try ONE_WAY_. */
            if (input.equals("ONE_WAY_CW")) {
                return ONE_WAY_CW;
            } else if (input.equals("ONE_WAY_CCW")) {
                return ONE_WAY_CCW;
            } else {
                return INVALID; /* Invalid if not ONE_SHOT_CW or ONE_SHOT_CCW. */
            }
        } else {
            return INVALID; /* Invalid if it starts with ONE_ but not the four above.  */
        }
    } else if (input.equals("BIDIRECTIONAL") || input.equals("SWEEP")) {  /* If it doesn't start w/ ONE_, try BIDIRECTIONAL. */
        return BIDIRECTIONAL;
    } else if (input.equals("FALLBACK")) { /* Otherwise, try FALLBACK. */
        return FALLBACK;
    } else {
        return INVALID; /* Otherwise, invalid. */
    }
}

void ESPServo::onTimerInstance() {
    /* Make sure angle-step isn't 0. If so, stop timer. */
    if (angleStep == 0) {
        Serial.println("Angle-step set at 0–stopping timer.");
        stop();
    }
    switch (currentMotion) {
        case ONE_SHOT_CW:
            /* Make sure angle-step is +. */
            if (angleStep < 0) angleStep *= -1; /* Increment clockwise. */
            if (pos + angleStep <= stopAngle) {
                pos += angleStep; /* Continue incrementing, setting new position. */
                writeCurrentPosition(); /* Update new position. */
            } else {
                stop(); /* Stop timer. */
            }
            return;
        case ONE_SHOT_CCW: /* One-shot motion counterclockwise. Moves at constant speed, starts at stopAngle, stops at startAngle.  */
            /* Make sure angle-step is -. */
            if (angleStep > 0) angleStep *= -1;
            /* Increment ccw. */
            if (pos + angleStep >= startAngle) {
                /* Continue incrementing, setting new position. */
                pos += angleStep;
                /* Write position. */
                writeCurrentPosition();
            } else {
                /* Stop timer. */
                stop();
            }
            return;
        case ONE_WAY_CW:
            /* Make sure angle-step is +. */
            if (angleStep < 0) angleStep *= -1;
            /* Increment CW. */
            if (pos + angleStep <= stopAngle) {
                pos += angleStep;
            } else {
                /* Back to 0º. */
                pos = 0;
                prevMotion = ONE_WAY_CW;
                setMotion(FALLBACK);
            }
            /* Write position. */
            writeCurrentPosition();
            return;
        case ONE_WAY_CCW:
            /* Make sure angle-step is -. */
            if (angleStep > 0) angleStep *= -1;
            /* Increment CCW. */
            if (pos + angleStep >= startAngle) {
                pos += angleStep;
            } else {
                /* Back to max angle.*/
                pos = maxAngle;
                prevMotion = ONE_WAY_CCW;
                setMotion(FALLBACK);
            }
            /* Write new position. */
            writeCurrentPosition();
            return;
        case FALLBACK: {
            esp_rom_delay_us(1000000);
            setMotion(prevMotion);
        } break;
        case INVALID: {
            stop();
            Serial.println("Current motion is set INVALID.");
        } break;
        default:
            /* Bidirectional */
            /* Check direction. */
            if (angleStep < 0) {
                /* Currently decrementing. */
                if (pos + angleStep > startAngle) {
                    /* Continue decrement. */
                    pos += angleStep;
                } else {
                    /* Set position to 0. */
                    pos = 0;
                    /* Invert angle step. */
                    angleStep *= -1;
                }
            } else {
                if (pos + angleStep < stopAngle) {
                    /* Continue incrementing. */
                    pos += angleStep;
                } else {
                    /* Set position to max angle. */
                    pos = maxAngle;
                    /* Invert angle-step. */
                    angleStep *= -1;
                }
            }
            /* Update current position. */
            writeCurrentPosition();
            return;
    }
}

const std::map<String, uint8_t> ESPServo::pinMap {
        {"D0", D0},
        {"D1", D1},
        {"D2", D2},
        {"D3", D3},
        {"D4", D4},
        {"D5", D5},
        {"D6", D6},
        {"D7", D7},
        {"D8", D8},
        {"D9", D9},
        {"D10", D10},
        {"D11", D11},
        {"D12", D12},
        {"D13", D13}
};
ESPServo::ESPServo(motion_mode_t _motion, int _pos, int _angleStep, long timeDelay,
                   int _maxAngle, int _startAngle, int _stopAngle,
                   uint8_t _pin, uint32_t freq, uint8_t res,
                   unsigned long _minPWM, unsigned long _maxPWM)
        : pos(_pos), currentMotion(_motion), prevMotion(_motion), angleStep(_angleStep),
          startAngle(_startAngle), stopAngle(_stopAngle), pin(_pin), frequency(freq),
          resolution(res), minPWM(_minPWM), maxPWM(_maxPWM),
          timeDelayUS(timeDelay), maxAngle(_maxAngle), success(false), servoTimer(nullptr)

{

    servoArgs.callback = onTimer;
    servoArgs.arg = this;
    servoArgs.name = "servoTimerArgs";
    servoArgs.skip_unhandled_events = false;
    maxTicks = (1 << resolution) - 1;
    channel = digitalPinToAnalogChannel(pin);
    printUpdates = true;
}

bool ESPServo::attach(const uint8_t &_pin) {
    stop();

    if (servoTimer == nullptr) {
        servoArgs.callback = onTimer;
        servoArgs.arg = this;
        servoArgs.name = "servoTimerArgs";
        servoArgs.skip_unhandled_events = false;

        esp_err_t timerCreateErr = esp_timer_create(&servoArgs, &servoTimer);
        if (timerCreateErr != ESP_OK) {
            PRINTF_UPDATELN("Failed to create timer: %s", esp_err_to_name(timerCreateErr));
            return false;
        } else {
            PRINT_UPDATELN("Successfully created timer.");
        }
    }

    if (digitalPinIsValid(_pin)) {
        pin = _pin;
    } else {
        PRINTF_UPDATELN("Invalid pin %u.", _pin);
        return false;
    }

    channel = digitalPinToAnalogChannel(pin);
    uint32_t prevFreq = frequency;
    frequency = ledcSetup(channel, frequency, resolution);
    if (prevFreq != frequency && printUpdates) {
        PRINTF_UPDATELN("Input frequency was modified to %u Hz.", frequency);
    }

    if (frequency != 0) {
        ledcAttachPin(pin, channel);
        if (startAngle < 0) {
            PRINTF_UPDATELN("startAngle %d < 0, setting to 0.", startAngle);
            startAngle = 0;
        }
        pos = startAngle;
        writeCurrentPosition();
        esp_rom_delay_us(4000000);
        PRINT_UPDATELN("Set up complete.");
        return true;
    } else {
        PRINT_UPDATELN("Failed to setup servo.");
        return false;
    }
}

void ESPServo::start() {
    //PRINT_UPDATELN("In start...");
    if (esp_timer_is_active(servoTimer)) {
        PRINT_UPDATELN("Servo is already running.");
    } else {
        PRINT_UPDATELN("Starting servo... (delay: %lu µs)", timeDelayUS);
        esp_err_t err = esp_timer_start_periodic(servoTimer, timeDelayUS);
        if (err != ESP_OK) {
            Serial.printf("Failed to start timer: %s\n", esp_err_to_name(err));
        }
    }
}
bool ESPServo::stop() {
    //PRINT_UPDATELN("Attempting to stop servo...");
    bool current_active = esp_timer_is_active(servoTimer);
    if (current_active) {
        esp_err_t err = esp_timer_stop(servoTimer);
        if (err != ESP_OK) {
            PRINTF_UPDATELN("Failed to stop servo: %s", esp_err_to_name(err));
            PRINTF_UPDATELN("Attempting to restart....");
            esp_restart();
        } else {
            PRINT_UPDATELN("Stopped servo.");
        }
    } else {
        //PRINT_UPDATELN("Not currently active.");
    }
    return current_active;
}
bool ESPServo::setTimeDelayUS(const unsigned long &newTimeDelayUS) {
    bool stopped = stop();
    if (newTimeDelayUS < maxPWM) {
        PRINTF_UPDATELN("Provided delay '%ld' < required pulse-width %ld.", newTimeDelayUS, maxPWM);
        if (stopped) start();
        return false;
    } else {
        timeDelayUS = long(newTimeDelayUS);
        PRINTF_UPDATELN("Updated time-delay to %ld µs", timeDelayUS);
        if (stopped) start();
        return true;
    }
}
bool ESPServo::setTimeDelayMS(const unsigned long &newTimeDelayMS) {
    bool stopped = stop();
    if (newTimeDelayMS * 1000 < maxPWM) {
        PRINTF_UPDATELN("Provided delay (%ld µs) < required PWM signal of %ld µs.", newTimeDelayMS * 1000, maxPWM);
        if (stopped) start();
        return false;
    } else {
        timeDelayUS = long(newTimeDelayMS) * 1000;
        PRINTF_UPDATELN("Updated time-delay to %ld ms ( %ld µs).", newTimeDelayMS, timeDelayUS);
        if (stopped) start();
        return true;
    }
}
bool ESPServo::setAngleStep(const int &newAngleStep) {
    if (newAngleStep < maxAngle) {
        bool stopped = stop();
        if (newAngleStep == 0) { /* Determine if user wants to stop all motion. */
            if (esp_timer_is_active(servoTimer)) {
                PRINTF_UPDATELN("Stopped all servo motion, angleStep unchanged at %d º/ %ld µs.", angleStep, timeDelayUS); /* Inform user angleStep unchanged, but motion stopped.*/
                if (stopped) start();
                return false;
            } else {
                PRINTF_UPDATELN("Cannot set angleStep to 0. Angle-step left at %d º / %ld µs.", angleStep, timeDelayUS); /* Inform user angleStep unchanged.*/
                if (stopped) start();
                return false;
            }
        } else {
            angleStep = newAngleStep;
            PRINTF_UPDATELN("New angle-step set to %dº / %ld µs", angleStep, timeDelayUS); /* Inform user angleStep changed.*/
            if (stopped) start();
            return true;
        }
    } else {
        PRINTF_UPDATELN("Received angle-step '%d' exceeds maximum range of servo (%dº).", newAngleStep, maxAngle);
        return false;
    }
}
bool ESPServo::reconfigure(const uint8_t &newPin, const uint32_t &newFreq, const uint8_t &newRes) {
    uint32_t _newFreq = ledcSetup(digitalPinToAnalogChannel(newPin), newFreq, newRes);
    if (_newFreq != 0) {
        frequency = _newFreq;
        pin = newPin;
        channel = digitalPinToAnalogChannel(pin);
        ledcAttachPin(pin, channel);
        PRINT_UPDATELN("Reconfigure success: pin = %u, freq = %u, channel = %u", pin, frequency, channel);
        return true;
    }
    return false;
}
bool ESPServo::setPin(const uint8_t &newPin) {
    if (newPin == pin) return true;
    success = false;
    if (digitalPinHasPWM(digitalPinToGPIONumber(newPin))) {
        bool stopped = stop();
        if (reconfigure(newPin, frequency, resolution)) {
            success = true;
        }
        if (stopped) start();
    } else {
        PRINTF_UPDATELN("Invalid pin received: %d (not PWM capable).", newPin);
    }
    return success;
}
bool ESPServo::setPin(const String &newPin) {
    auto it = pinMap.find(newPin);
    success = false;
    if (it != pinMap.end()) {
        if (it->second == pin) return true;
        if (digitalPinHasPWM(it->second)) {
            bool stopped = stop();
            if (reconfigure(it->second, frequency, resolution)) {
                success = true;
                pin = it->second;
            }
            if (stopped) start();
        } else {
            PRINTF_UPDATELN("Pin '%s' isn't PWM capable.", newPin.c_str());
        }
    } else {
        PRINTF_UPDATELN("Invalid pin '%s'.", newPin.c_str());
    }
    return success;
}

bool ESPServo::setAngle(const int &newAngle) {
    if (newAngle == pos) return true;
    if (0 <= newAngle && newAngle <= maxAngle) {
        bool stopped = stop();
        pos = newAngle;
        writeCurrentPosition();
        if (stopped) start();
        return true;
    } else {
        PRINTF_UPDATELN("Angle '%d' must be between 0 and %d.", newAngle, maxAngle);
        return false;
    }
}
/**
 * Function to set the motion of servo motor. Stops if running, updates motion, and starts back up if it had been running.
 * @param motion is the motion to set for the servo's actuation.
 */
void ESPServo::setMotion(const motion_mode_t &motion) {
    if (motion == currentMotion) return;
    bool stopped = stop();
    /* No need to delete timer. Arguments still the same. */
    //prevMotion = currentMotion;
    currentMotion = motion;
    PRINTF_UPDATELN("New motion set: %s", motionToString(currentMotion));
    if (stopped) start();
}

bool ESPServo::setMaxAngle(const int &newAngle) {
    if (newAngle == maxAngle) return true;
    if (newAngle > 0) {
        bool stopped = stop();
        maxAngle = newAngle;
        PRINTF_UPDATELN("New max angle set to %dº.", maxAngle);
        if (stopped) start();
        return true;
    } else {
        PRINTF_UPDATELN("Max angle must be positive.");
        return false;
    }
}
bool ESPServo::setMinPWM(const int &newPWM) {
    if (newPWM == minPWM) return true;
    if (newPWM > 0) {
        bool stopped = stop();
        minPWM = newPWM;
        PRINTF_UPDATELN("New PWM min set to %ld µs.", minPWM);
        if (stopped) start();
        return true;
    } else {
        PRINTF_UPDATELN("New PWM must be positive.");
        return false;
    }
}
/**
 * Function to set the maximum pulse-width for PWM signal generation.
 * @param newPWM is the
 */
bool ESPServo::setMaxPWM(const unsigned long &newPWM) {
    if (newPWM == maxPWM) return true;
    if (newPWM > minPWM) {
        bool stopped = stop();
        maxPWM = newPWM;
        PRINTF_UPDATELN("New PWM min set to %ld µs.", maxPWM);
        if (stopped) start();
        return true;
    } else {
        PRINTF_UPDATELN("New PWM must be > minPWM (%ld).", minPWM);
        return false;
    }
}

bool ESPServo::setStartAngle(const int &_startAngle) {
    if (_startAngle == startAngle) return true;
    if (_startAngle <= stopAngle) {
        bool stopped = stop();
        startAngle = _startAngle;
        PRINTF_UPDATELN("New start-angle set to %dº.", startAngle);
        if (stopped) start();
        return true;
    } else {
        PRINTF_UPDATELN("Received stop-angle '%d' exceeds stop-angle (%dº).", _startAngle, stopAngle);
        return false;
    }
}
int ESPServo::getStartAngle() const {
    return startAngle;
}

/**
 * Function to set the stopping angle for motion. NOT used for PWM signal generation.
 * Stops motor if it was running, and starts if it was stopped.
 * @param newAngle is the new stopping angle [º].
 */
bool ESPServo::setStopAngle(const int &newAngle) {
    if (stopAngle == newAngle) return true;
    success = false;
    if (newAngle <= maxAngle) {
        bool stopped = stop();
        stopAngle = newAngle;
        PRINTF_UPDATELN("New stop-angle set to %dº.", stopAngle);
        if (stopped) start();
        success = true;
    } else {
        PRINTF_UPDATELN("Received stop-angle '%d' exceeds maximum range of servo (%dº).", newAngle, maxAngle);
    }
    return success;
}
/**
 * Method to write the angle defined by 'pos' to the servo.
 */
void ESPServo::writeCurrentPosition() const {
    long period = 1000000 / long(frequency);
    int pulse = map(pos, 0, maxAngle, static_cast<long>(minPWM), static_cast<long>(maxPWM));
    int duty = (pulse * maxTicks / period);
    ledcWrite(channel, duty);
    PRINTF_UPDATELN("Current angle: %d", pos);
    delayMicroseconds(pulse);
}

bool ESPServo::setFrequency(const uint32_t &newFrequency) {
    if (frequency == newFrequency) return true;
    uint32_t prevFrequency = frequency;
    bool stopped = stop();
    success = false;
    frequency = ledcSetup(channel, newFrequency, resolution);
    if (frequency == 0) {
        frequency = prevFrequency;
        PRINTF_UPDATELN("Failed to set frequency to %u Hz.", newFrequency);
    } else {
        success = true;
        PRINTF_UPDATELN("New frequency set to %u Hz.", frequency);
    }
    ledcAttachPin(pin, channel);
    if (stopped) start();
    return success;
}

bool ESPServo::setResolution(const uint8_t &newRes) {
    if (newRes == resolution) return true;
    uint8_t prevRes = resolution;
    bool stopped = stop();
    uint32_t newFreq = ledcSetup(channel, frequency, newRes);
    if (newFreq == 0) {
        PRINTF_UPDATELN("Failed to set resolution to %u bits.", newRes);
        if (stopped) start();
        return false;
    } else {
        resolution = newRes;
        ledcAttachPin(pin, channel);
        if (stopped) start();
        return true;
    }
}