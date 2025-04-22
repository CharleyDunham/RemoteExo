//
// Created by Sullivan Bryant on 4/9/25.
//

#ifndef REMAPPINSTRING_H
#define REMAPPINSTRING_H
#include <Arduino.h>
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

#endif //REMAPPINSTRING_H