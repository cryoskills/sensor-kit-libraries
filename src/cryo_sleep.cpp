/*****************************************************************************

MIT License

Copyright (c) 2024 Cardiff University / cryoskills.com

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*****************************************************************************/

#include <Arduino.h>
#include "SPI.h"
#include "SD.h"

#include "cryo_sleep.h"

PseudoRTC cryo_rtc;
volatile boolean cryo_asleep_flag_debug = false;

PseudoRTC::PseudoRTC() {
    // Initialise all alarms
    for (uint8_t k = 0; k < MAX_RTC_ALARMS; k++) {
        this->remove_alarm(k);
    }
}

uint16_t PseudoRTC::month_from_str(const char* year_str) {
    for (int j = 0; j < 48; j += 4) {
        if (!strcmp(PseudoRTC::NAMES_OF_MONTH + j, year_str)) {
            return (j/4);
        }
    }
    return 0xffff;
}

void PseudoRTC::tick() {

    this->second += CRYO_SLEEP_INTERVAL_SECONDS;
    // Increment minute
    if (this->second > 59) {
        this->second = (this->second + CRYO_SLEEP_INTERVAL_SECONDS) % 60;
        this->minute += 1;
    }
    // Increment hour
    if (this->minute > 59) {
        this->hour += 1;
        this->minute = 0;
    }
    // Increment day
    if (this->hour > 23) {
        this->day += 1;
        this->hour = 0;
    }

    // Increment month
    if (this->month == 1 && PseudoRTC::is_leap_year(this->rtc_time) && this->day > 29) {
        this->month += 1;
        this->day = 1;
    // not nice, but make implicit correction for -1 index here so that 
    // the days_of_month array is logical
    } else if (this->day > PseudoRTC::DAYS_OF_MONTH[this->month]) {
        this->month += 1;
        this->day = 1;
    }

    // Increment year
    if (this->month > 11) {
        this->year += 1;
        this->month = 0;
    }

    if (this->year > 9999) {
        this->year = 0;
    };

    // Update alarm values (but don't run them as we're still in the ISR)
    this->check_alarms();

}

PseudoRTC::time PseudoRTC::get_time () {
    return this->rtc_time;
}

void PseudoRTC::set_time(PseudoRTC::time time) {
    this->rtc_time = time;
}

void PseudoRTC::set_time_from_compile_headers(const char* date, const char* time) {

    char month_buffer[4];
    PseudoRTC::time new_time;
    // Read date
    sscanf(date, "%s %d %d", month_buffer, &new_time.day, &new_time.year);
    new_time.month = PseudoRTC::month_from_str(month_buffer);
    // Read time
    sscanf(time, "%d:%d:%ud", &new_time.hour, &new_time.minute, &new_time.second);

    this->set_time(new_time);    

}

bool PseudoRTC::is_leap_year(PseudoRTC::time time) {

    // source: https://airandspace.si.edu/stories/editorial/science-leap-year

    // Initial check, is the year divisible by four
    bool leap_year_flag = time.year % 4 == 0;

    // however, if it is divisible by one hundred by not divisible by four
    // then it's not a leap year
    if ((time.year % 100 == 0) && (time.year % 400 != 0))
        leap_year_flag = false;

    return leap_year_flag;

}

void PseudoRTC::check_alarms() {

    // this function should be called every tick()

    // Update alarms
    for (uint8_t k = 0; k < MAX_RTC_ALARMS; k++) {
        //  check if alarm is null-ptr
        if (this->alarm_callback[k] != NULL) {
            // increment count
            this->alarm_counts[k]++;
            // check against interval
            if (this->alarm_counts[k] >= this->alarm_intervals[k]) {
                // set the alarm flag (don't clear this until we've called the alarm)
                this->alarm_flags[k] = 1;
                // and clear the count to start again
                this->alarm_counts[k] = 0;
            }
        }
    }
}

void PseudoRTC::raise_alarms() {

    // Iterate over alarms
    for (uint8_t k = 0; k < MAX_RTC_ALARMS; k++) {
        if (this->alarm_callback[k] != NULL && this->alarm_flags[k]) {
            // call the alarm
            this->alarm_flags[k] = 0;
            this->alarm_callback[k]();
        }
    }

}

uint8_t PseudoRTC::add_alarm_every_n_seconds(uint32_t interval, void (*callback)()) {

    // Iterate through alarm looking for next NULL ptr
    for (uint8_t k = 0; k < MAX_RTC_ALARMS; k++) {
        if (this->alarm_callback[k] == NULL) {
            this->alarm_callback[k] = callback;
            this->alarm_intervals[k] = interval;
            this->alarm_counts[k] = 0;
            this->alarm_flags[k] = 0;
            return k;
        }
    }
    // return 0xff on no alarms set
    return 0xff;

}

void PseudoRTC::remove_alarm(uint8_t alarm_id) {
    
    // don't do anything if the alarm_id is invalid 
    if (alarm_id > MAX_RTC_ALARMS - 1)
        return;

    // otherwise, reset the callback function to NULL and clear all flags/counters
    this->alarm_callback[alarm_id] = NULL;
    this->alarm_flags[alarm_id] = 0;
    this->alarm_intervals[alarm_id] = 0;
    this->alarm_counts[alarm_id] = 0;

}


uint8_t PseudoRTC::get_timestamp(char* str) {
    sprintf(
        str,
        // results in a string that is 
        // 3 + 3 + 5 + 3 + 3 + 2 + 3
        // = 22 length, say 24 to be safe
        "%02d-%02d-%04d %02d:%02d:%02d", 
        this->day,
        this->month+1,
        // Move from name of month to numberic format
        // &NAMES_OF_MONTH[4*(this->month)],
        this->year,
        this->hour,
        this->minute,
        this->second
    );
    return strlen(str);
}

void cryo_configure_clock(const char* date, const char* time) {
    // 
    zpmRTCInit();
    // PseudoRTC::time init_time = {
    //     year : 2024,
    //     month : 2,
    //     day : 6,
    //     hour : 17,
    //     minute : 13,
    //     second : 10
    // };
    // cryo_rtc.set_time(init_time);
    cryo_rtc.set_time_from_compile_headers(date, time);
    zpmRTCInterruptEvery(1024 * CRYO_SLEEP_INTERVAL_SECONDS, cryo_rtc_handler);

}

void cryo_wakeup() {

    #ifdef CRYO_SLEEP_MODE_DEBUG
        cryo_wakeup_debug()
    #else

        zpmCPUClk48M();
        // Removed 48M clock as this appeared to be causing the device to hang
        // but stable now on transmitter

    #endif

}
void cryo_wakeup_debug() {}; // do nothing

void cryo_raise_alarms() {
    
    // check alarms
    cryo_rtc.raise_alarms();
    
}

void cryo_add_alarm_every(uint32_t seconds, void (*callback)()) {

    cryo_rtc.add_alarm_every_n_seconds(seconds, callback);

}

void cryo_sleep() {

    #ifdef CRYO_SLEEP_MODE_DEBUG
        cryo_sleep_debug()
    #else
        cryo_asleep_flag_debug = true;
        // Removed sleep/interrupt masks
        
        SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;	
        zpmCPUClk32K();
        zpmSleep();
        // SCB->SCR |= SCB_SCR_SLEEPONEXIT_Msk;
        // __DSB();
        // __WFE();
        SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
    
    #endif

}
void cryo_sleep_debug() {
    cryo_asleep_flag_debug = true;
    while (cryo_asleep_flag_debug) {}; 
}; // do nothing

void cryo_rtc_handler() {
    
    // Perform RTC tick
    cryo_rtc.tick();
    cryo_asleep_flag_debug = false;

}

void cryo_rtc_sd_callback(uint16_t* date, uint16_t* time) {
    /* Reference (modified from)
        https://forum.arduino.cc/t/file-creation-date-and-time-in-sd-card/336037/5
    */

    // return date using FAT_DATE macro to format fields
    *date = FAT_DATE(cryo_rtc.year, cryo_rtc.month, cryo_rtc.day);

    // return time using FAT_TIME macro to format fields
    *time = FAT_TIME(cryo_rtc.hour, cryo_rtc.minute, cryo_rtc.second);
}
//--------------

PseudoRTC* cryo_get_rtc() {
    return &cryo_rtc;
}

