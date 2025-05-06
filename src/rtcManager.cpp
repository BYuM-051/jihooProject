#include "rtcManager.h"

RTCManager::RTCManager()
{
    
}

void RTCManager::begin(uint8_t rst, uint8_t data, uint8_t clk) {
    this->rtc = new DS1302(rst, data, clk);
}

Time RTCManager::now() {
    return this->rtc->getTime();
}

void RTCManager::setTime(uint8_t hour, uint8_t min, uint8_t sec) {
    this->rtc->setTime(hour, min, sec);
}

void RTCManager::setDate(uint8_t date, uint8_t month, uint16_t year) {
    this->rtc->setDate(date, month, year);
}

void RTCManager::setDOW(uint8_t dow) {
    this->rtc->setDOW(dow);
}

bool RTCManager::isSameTime(const Time& t1, const Time& t2) {
    return (t1.hour == t2.hour &&
            t1.min == t2.min &&
            t1.sec == t2.sec);
}

void RTCManager::printTime(const Time& time, HardwareSerial& serial) const {
    serial.print("Time: ");
    if (time.hour < 10) serial.print('0');
    serial.print(time.hour);
    serial.print(":");
    if (time.min < 10) serial.print('0');
    serial.print(time.min);
    serial.print(":");
    if (time.sec < 10) serial.print('0');
    serial.println(time.sec);
}

void RTCManager::printDate(const Time& time, HardwareSerial& serial) const {
    serial.print("Date: ");
    if (time.mon < 10) serial.print('0');
    serial.print(time.mon);
    serial.print('/');
    if (time.date < 10) serial.print('0');
    serial.print(time.date);
    serial.print('/');
    serial.println(time.year);
}