#include <Arduino.h>

#ifndef __CRYO_SLEEP_H
#define __CRYO_SLEEP_H

class PseudoRTC {

    public: 

        struct time {
            uint32_t year;
            uint32_t month;
            uint32_t day;
            uint32_t hour;
            uint32_t minute;
            uint32_t second;
        } rtc_time;

        // Create reference variables to rtc_time members
        uint32_t& year = rtc_time.year;
        uint32_t& month = rtc_time.month;
        uint32_t& day = rtc_time.day;
        uint32_t& hour = rtc_time.hour;
        uint32_t& minute = rtc_time.minute;
        uint32_t& second = rtc_time.second;

        void tick();

        PseudoRTC::time get_time();
        void set_time(PseudoRTC::time time);
        void set_time_from_compile_headers();

        static bool is_leap_year(PseudoRTC::time time);

    private:
        const uint8_t DAYS_OF_MONTH[12] = {
            31, // Jan
            28, // Feb
            31, // Mar
            30, // April
            31, // May
            30, // June
            31, // July
            31, // August
            30, // Sept
            31, // Oct
            30, // Nov
            31  // Dec
        };
        const char* NAMES_OF_MONTH = "Jan\0Feb\0Mar\0Apr\0May\0Jun\0Jul\0Aug\0Sep\0Oct\0Nov\0Dec\0"; 

        uint16_t month_from_str(const char* year_str);

};

void cryo_rtc_handler();
void cryo_configure_clock();
void cryo_wakeup();
void cryo_sleep();

#endif

