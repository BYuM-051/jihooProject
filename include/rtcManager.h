
#ifndef RTC_MANAGER_H
#define RTC_MANAGER_H

#include <Arduino.h>
#include "DS1302.h"

class RTCManager {
public:
    RTCManager();
    void begin(uint8_t rst, uint8_t data, uint8_t clk);
    Time now();
    void setTime(uint8_t hour, uint8_t min, uint8_t sec);
    void setDate(uint8_t date, uint8_t mon, uint16_t year);
    void setDOW(uint8_t dow);

    static bool isSameTime(const Time &t1, const Time &t2);
    void printTime(const Time &t, HardwareSerial &serial) const;
    void printDate(const Time &t, HardwareSerial &serial) const;


private:
    DS1302* rtc;
};

#endif // RTC_MANAGER_H