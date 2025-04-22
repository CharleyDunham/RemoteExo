// SensorSampler.h
#pragma once
#include <Arduino.h>
#include <functional>
#include <variant>

using SampleSet      = std::array<uint16_t, 4>;

using SampleCallback = std::function<void(const SampleSet&)>;

class SensorSampler {
public:
    enum PIN_MODE { NOT_CONNECTED };
    typedef std::variant<uint8_t, PIN_MODE> pin;
    explicit SensorSampler(const std::array<pin, 4>& pins, uint64_t periodUs);
    ~SensorSampler();
    bool begin();
    void stop() const;
    void service();
    void onSample(SampleCallback cb) { callback_ = std::move(cb); }
    void setSamplingRate(unsigned long rate);
    [[nodiscard]] unsigned long getSamplingRate() const { return periodUs_; }
    // throws illegal argument exception.
    void setPin(unsigned int fingerIndex, pin p);
    void start() const; // throws runtime_exception
    bool listener() const { return callback_ != nullptr; }
private:

    std::array<pin, 4> pins_;
    std::array<uint16_t, 4> readings_;
    uint64_t             periodUs_;
    esp_timer_handle_t   timer_{nullptr};
    volatile bool        ready_{false};
    SampleCallback       callback_;
    static void IRAM_ATTR timerFn(void* arg);
};
