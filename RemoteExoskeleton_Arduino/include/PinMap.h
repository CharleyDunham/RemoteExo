//
// Created by Sullivan Bryant on 4/24/25.
//

#ifndef PINMAP_H
#define PINMAP_H
#include <Arduino.h>
#include <cstdlib>
#include <cctype>

static const char *getRemap(const uint8_t &pin) {
    switch (pin) {
        case A0: return "A0";
        case A1: return "A1";
        case A2: return "A2";
        case A3: return "A3";
        case A4: return "A4";
        case A5: return "A5";
        case A6: return "A6";
        case A7: return "A7";
        case D0: return "D0";
        case D1: return "D1";
        case D2: return "D2";
        case D3: return "D3";
        case D4: return "D4";
        case D5: return "D5";
        case D6: return "D6";
        case D7: return "D7";
        case D8: return "D8";
        case D9: return "D9";
        case D10: return "D10";
        case D11: return "D11";
        case D12: return "D12";
        case D13: return "D13";
        default: return "";
    }
}
// returns -1 if invalid.
static int8_t getMap(const char *pin) {
    // Check if null or empty.
    if (!pin || !pin[0])
        return -1;
    char letter = pin[0];
    char *endptr = nullptr;
    long num = std::strtol(pin + 1, &endptr, 10);
    if (endptr == pin + 1 || *endptr != '\0')
        return -1;
    if (letter == 'A') {
        if (num >= 0 && num <= 7)
            return A0 + num;
    }
    else if (letter == 'D') {
        if (num >= 0 && num <= 13)
            return num;
    }
    return -1;  // invalid
}

#endif //PINMAP_H
