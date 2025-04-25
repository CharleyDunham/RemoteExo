// SensorSampler.h
#pragma once
#include <Arduino.h>
#include <functional>
#include <variant>
#include <SerialStream.h>
#include <PinMap.h>

using SampleSet      = std::array<uint16_t, 4>;
using SampleCallback = std::function<void(SampleSet)>;

class SensorSampler {
public:
    enum PIN_MODE { NOT_CONNECTED };
    //typedef std::variant<uint8_t, PIN_MODE> pin_t;
    explicit SensorSampler(const std::array<std::optional<uint8_t>, 4>& pins, uint64_t periodUs);
    ~SensorSampler();
    bool begin();
    void stop() const;
    void service();
    void onSample(SampleCallback cb) { callback_ = std::move(cb); }
    void setSamplingRate(unsigned long rate);
    [[nodiscard]] unsigned long getSamplingRate() const { return periodUs_; }
    // throws illegal argument exception.
    enum FingerIndex {
        INDEX = 2,
        MIDDLE = 3,
        RING = 4,
        PINKY = 5
    };

    void setPin(FingerIndex index, std::optional<uint8_t> p);
    [[nodiscard]] std::optional<uint8_t> getPin(FingerIndex index) const;
    void start(); // throws runtime_exception
    [[nodiscard]] bool listener() const { return callback_ != nullptr; }
private:
    std::array<std::optional<uint8_t>, 4> pins_;
    std::array<uint16_t, 4> readings_;
    uint64_t             periodUs_;
    esp_timer_handle_t   timer_{nullptr};
    volatile bool        ready_{false};
    SampleCallback       callback_;
    static void IRAM_ATTR timerFn(void* arg);
};
