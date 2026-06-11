#ifndef APP_TYPES_H
#define APP_TYPES_H

#include <stdint.h>
#include <stdbool.h>

struct RtcDateTime {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
};

struct SensorData {
    uint32_t seq;
    bool rtc_ok;
    bool bme_ok;
    struct RtcDateTime rtc;
    int16_t temperature_c_x100;
    uint16_t humidity_pct_x100;
    uint16_t pressure_mmhg;
};

#endif // APP_TYPES_H
