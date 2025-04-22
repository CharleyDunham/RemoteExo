//
// Created by Sullivan Bryant on 4/20/25.
//

#ifndef SERIALSTREAM_H
#define SERIALSTREAM_H
#pragma once
#include <Arduino.h>
// Generic template for anything Print::print() handles
template <typename T>
inline Print& operator<<(Print &out, const T& value) {
    out.print(value);
    return out;
}
inline Print& operator<<(Print& out, const __FlashStringHelper* v) {
    out.print(v);
    return out;
}
inline Print& endl(Print& out) {
    out.println();
    return out;
}
inline Print& operator<<(Print& out, Print& (*manip)(Print&)) {
    return manip(out);
}

#endif //SERIALSTREAM_H
