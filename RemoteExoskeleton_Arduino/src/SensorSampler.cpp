// SensorSampler.cpp
#include "SensorSampler.h"
#include <esp_timer.h>

SensorSampler::SensorSampler(const std::array<std::optional<uint8_t>, 4>& pins, const uint64_t periodUs)
  : pins_(pins), periodUs_(periodUs), readings_({0, 0, 0, 0}) {}

SensorSampler::~SensorSampler() {
    if (timer_) {
        esp_timer_stop(timer_);
        esp_timer_delete(timer_);
    }
}

bool SensorSampler::begin() {
    for (auto& vp : pins_) {
        auto val = vp.value_or(false);
        if (val != false) {
            if (!adcAttachPin(vp.value())) {
                Serial << "Failed to attach pin " << vp.value() << "\n";
                return false;
            }
        }
        // else: NOT_CONNECTED, skip
    }

    esp_timer_create_args_t args = {
        .callback            = &SensorSampler::timerFn,
        .arg                 = this,
        .dispatch_method     = ESP_TIMER_TASK,
        .name                = "flexSample",
        .skip_unhandled_events = false
      };
    if (esp_err_t err = esp_timer_create(&args, &timer_); err != ESP_OK) {
        if (err == ESP_ERR_NO_MEM) {
            Serial << "Failed to create timer - SensorSampler - line 35:" << endl;
            Serial << " >> Memory allocation for timer failed." << endl;
        } else if (err == ESP_ERR_INVALID_STATE) {
            Serial << "Failed to create timer - SensorSampler - line 35:" << endl;
            Serial << " >> Timer library not initialized yet." << endl;
        } else {
            Serial << "Failed to create timer - SensorSampler - line 35:" << endl << " >> Some of arguments for esp_timer_create in begin() method for SensorSampler"
                      "are invalid." << endl;
        }
        return false;
    }
    Serial << "Successfully initialized SensorSampler sampling timer." << endl;
    return true;
}

void SensorSampler::stop() const {
    if (timer_) esp_timer_stop(timer_);
}
void SensorSampler::start() {
    if (!timer_) {
        if (!begin()) {
            Serial << "Initial failure to start sensor timer. Waiting 500,000 us and trying again." << endl;
            esp_rom_delay_us(500000);
            if (!begin()) {
                Serial << "Failed to start sensor sampler. Not starting." << endl;
                return;
            }
            Serial << "Sensor sampler started." << endl;
        } else {
            esp_err_t err = esp_timer_start_periodic(timer_, periodUs_);
            if (err != ESP_OK) {
                Serial << "Failed to start sensor timer: " << esp_err_to_name(err) << endl;
            }
        }
    } else {
        if (esp_err_t err = esp_timer_start_periodic(timer_, periodUs_); err != ESP_OK) {
            Serial << "Failed to restart sensor timer: " << esp_err_to_name(err) << endl;
        }
    }
}

void SensorSampler::service() {
    if (!ready_) return;
    ready_ = false;
    int i = 0;
    for (auto pin : pins_) {
        if (pin.has_value()) {
            readings_[i] = analogRead(pin.value());
        } else {
            readings_[i] = 0;
        }
        i++;
    }
    if (callback_) callback_(readings_);
}

void IRAM_ATTR SensorSampler::timerFn(void* arg) {
    auto* self = static_cast<SensorSampler*>(arg);
    self->ready_ = true;
}

void SensorSampler::setSamplingRate(const unsigned long rate) {
    const bool isActive = esp_timer_is_active(timer_);
    stop();
    if (rate < 100000UL) {
        Serial << "sampling rate is too frequent." << endl;
        return;
    }
    periodUs_ = rate;
    if (isActive) begin();
}

void SensorSampler::setPin(FingerIndex index,  std::optional<uint8_t> p) {
    const bool active = esp_timer_is_active(timer_);
    stop();
    pins_[index] = p;
    begin();
    if (active) start();
}

std::optional<uint8_t> SensorSampler::getPin(FingerIndex index) const {
    return pins_[index];
}
