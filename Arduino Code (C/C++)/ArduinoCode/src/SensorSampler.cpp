// SensorSampler.cpp
#include "SensorSampler.h"
#include <esp_timer.h>

SensorSampler::SensorSampler(const std::array<pin, 4>& pins, const uint64_t periodUs)
  : pins_(pins), periodUs_(periodUs), readings_({0, 0, 0, 0}) {}

SensorSampler::~SensorSampler() {
    if (timer_) {
        esp_timer_stop(timer_);
        esp_timer_delete(timer_);
    }
}

bool SensorSampler::begin() {
    for (auto& vp : pins_) {
        if (auto pinPtr = std::get_if<uint8_t>(&vp)) {
            uint8_t pin = *pinPtr;
            if (!adcAttachPin(pin)) return false;
            pinMode(pin, INPUT);
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
    if (esp_timer_create(&args, &timer_) != ESP_OK) return false;
    return esp_timer_start_periodic(timer_, periodUs_) == ESP_OK;
}

void SensorSampler::stop() const {
    if (timer_) esp_timer_stop(timer_);
}
void SensorSampler::start() const {
    if (timer_) {
        esp_err_t err = esp_timer_start_periodic(timer_, periodUs_);
        if (err != ESP_OK) {
            throw std::runtime_error("Failed to start timer");
        }
    }
}

void SensorSampler::service() {
    if (!ready_) return;
    ready_ = false;
    for (size_t i = 0; i < pins_.size(); ++i) {
        auto& p = pins_[i];
        if (const auto ptr = std::get_if<uint8_t>(&p)) {
            readings_[i] = analogRead(*ptr);
        } else {
            readings_[i] = 0;
        }
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
    if (rate < 100000UL) throw std::invalid_argument("sampling rate is too frequent.");
    periodUs_ = rate;
    if (isActive) begin();
}
void SensorSampler::setPin(unsigned int fingerIndex, pin p) {
    if (fingerIndex >= pins_.size()) throw std::invalid_argument("invalid finger index");
    const bool active = esp_timer_is_active(timer_);
    stop();
    pins_[fingerIndex] = p;
    if (active) begin();
}
