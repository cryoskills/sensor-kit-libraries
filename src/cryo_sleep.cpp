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
#include "cryo_system.h"

// uint16_t[] PseudoRTC::NAMES_OF_MONTH;
const uint8_t PseudoRTC::DAYS_OF_MONTH[12] = {
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

const uint16_t PseudoRTC::PREVIOUS_DAYS_BY_MONTH[12] = {
    0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
};

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
        this->second = (this->second) % 60;
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

    PseudoRTC::time new_time;
    //
    get_time_from_compile_headers(date, time, &new_time);
    this->set_time(new_time);    

}

void PseudoRTC::get_time_from_compile_headers(const char* date, const char* time, PseudoRTC::time* time_object) {

    char month_buffer[4];
    // Read date
    CRYO_DEBUG_MESSAGE("Converting date");
    sscanf(date, "%s %d %d", month_buffer, (int*)&time_object->day, (int*)&time_object->year);
    time_object->month = PseudoRTC::month_from_str(month_buffer);
    // Read time
    CRYO_DEBUG_MESSAGE("Converting time");
    sscanf(time, "%d:%d:%d", (int*)&time_object->hour, (int*)&time_object->minute, (int*)&time_object->second);

}


uint64_t PseudoRTC::time_to_seconds(PseudoRTC::time time) {

    uint64_t time_in_seconds;
    uint8_t leap_days;

    // Calculate leap days since origin (1st Jan 2000);
    leap_days = time.year / 4 - time.year / 100; 

    // Calculate time in seconds (Wihout leap years)
    time_in_seconds = 60 * (
        time.second + 
        60 * (
            time.minute +
            24 * (
                time.day + 
                leap_days + 
                PseudoRTC::PREVIOUS_DAYS_BY_MONTH[time.month] + 
                365 * time.year 
            )
        )
    );

    return time_in_seconds;
    
}

int8_t PseudoRTC::in_chronological_order(PseudoRTC::time time_a, PseudoRTC::time time_b) {

    uint64_t time_a_seconds = PseudoRTC::time_to_seconds(time_a);
    uint64_t time_b_seconds = PseudoRTC::time_to_seconds(time_b);

    if (time_b_seconds > time_a_seconds) {
        CRYO_DEBUG_MESSAGE("time b > time a");
        return 1;
    } else if (time_b_seconds < time_a_seconds) {
        CRYO_DEBUG_MESSAGE("time a > time b");
        return -1;
    }
    
    CRYO_DEBUG_MESSAGE("time b = time a");
    return 0;

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

uint8_t PseudoRTC::get_timestamp_compiler_format(char* str) {
    
    sprintf(
        str,
        // results in a string that is 
        // 3 + 3 + 5 + 3 + 3 + 2 + 3
        // = 22 length, say 24 to be safe
        "%s %02d %04d %02d:%02d:%02d", 
        &NAMES_OF_MONTH[4*(this->month)],
        this->day,
        this->year,
        this->hour,
        this->minute,
        this->second
    );
    return strlen(str);
}

void cryo_configure_clock(const char* date, const char* time) {
    
    //  Init variable to store SD card time
    PseudoRTC::time sd_time;
    PseudoRTC::time compile_time;
    // Assume a fail, only set to true once time succesfully set
    bool sd_clock_fail = true;
    
    // CRYO_DEBUG_MESSAGE("Enable OSC32K and run in standby");
    // // keep the XOSC32K running in standy
    // SYSCTRL->OSC32K.reg |= SYSCTRL_OSC32K_ENABLE;
    // SYSCTRL->OSC32K.reg |= SYSCTRL_OSC32K_RUNSTDBY;

    // CRYO_DEBUG_MESSAGE("Attach GCLK_RTC to generic clock generator 1");
    // // attach GCLK_RTC to generic clock generator 1
    // GCLK->CLKCTRL.reg = (uint32_t)((GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK1 | (RTC_GCLK_ID << GCLK_CLKCTRL_ID_Pos)));

    // CRYO_DEBUG_MESSAGE("Configuring OSC32K as GCLK 1 source");
    // // 
    // GCLK->GENCTRL.reg = GCLK_GENCTRL_ID(1) |
    //                    GCLK_SOURCE_OSC32K |
    //                    GCLK_GENCTRL_IDC   |
    //                    GCLK_GENCTRL_GENEN ;
    // while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY);

    // CRYO_DEBUG_MESSAGE("zpmRTCInit");
    // 
    zpmRTCInit();
    
    // Read SD clock time
    CRYO_DEBUG_MESSAGE("Converting compiler headers to timestamp");
    PseudoRTC::get_time_from_compile_headers(date, time, &compile_time);
    
    CRYO_DEBUG_MESSAGE("Beginning read from SD card");
    if (cryo_rtc.read_from_sd(CLOCK_FILENAME, &sd_time)) {
        
        CRYO_DEBUG_MESSAGE("SD card read successful");
        sd_clock_fail = false;

    }

    char local_buffer[9] = "HH:MM:SS";
    sprintf(local_buffer, "%02d:%02d:%02d", compile_time.hour, compile_time.minute, compile_time.second);
    CRYO_DEBUG_MESSAGE("Compiler");
    CRYO_DEBUG_MESSAGE(local_buffer);
    sprintf(local_buffer, "%02d:%02d:%02d", sd_time.hour, sd_time.minute, sd_time.second);
    CRYO_DEBUG_MESSAGE("SD");
    CRYO_DEBUG_MESSAGE(local_buffer);
    
    if (sd_clock_fail || PseudoRTC::in_chronological_order(sd_time, compile_time) > 0) {
        CRYO_DEBUG_MESSAGE("----------------------------");
        CRYO_DEBUG_MESSAGE("Using compiler provided time");
        cryo_rtc.set_time(compile_time);
        char loaded_timestamp[CRYO_RTC_TIMESTAMP_LENGTH];
        cryo_rtc.get_timestamp(loaded_timestamp);
        CRYO_DEBUG_MESSAGE("Writing compile time to SD card");
        cryo_rtc.write_to_sd(CLOCK_FILENAME);
        CRYO_DEBUG_MESSAGE(loaded_timestamp)
        CRYO_DEBUG_MESSAGE("----------------------------");
    } else {
        CRYO_DEBUG_MESSAGE("----------------------------");
        CRYO_DEBUG_MESSAGE("Using SD provided time")
        cryo_rtc.set_time(sd_time);
        char loaded_timestamp[CRYO_RTC_TIMESTAMP_LENGTH];
        cryo_rtc.get_timestamp(loaded_timestamp);
        CRYO_DEBUG_MESSAGE(loaded_timestamp)
        CRYO_DEBUG_MESSAGE("----------------------------");
    }

    zpmRTCInterruptEvery(1024 * CRYO_SLEEP_INTERVAL_SECONDS, cryo_rtc_handler);

}

uint8_t PseudoRTC::read_from_sd(const char* filename, PseudoRTC::time* sd_time) {
    
    File sd_clock_obj;
    char sd_clock_date[11];
    char sd_clock_time[8];

    CRYO_DEBUG_MESSAGE("Initialising SD card");
    if (!SD.begin(SD_CHIP_SELECT)) {
        CRYO_DEBUG_MESSAGE("No SD card for RTC time");
        return 0;
    }
    
    CRYO_DEBUG_MESSAGE("Checking for RTC clock file");
    if (!SD.exists(filename)) {
        CRYO_DEBUG_MESSAGE("No clock file on SD card for RTC time");
        return 0;
    }


    CRYO_DEBUG_MESSAGE("Reading date from SD card");
    // Try reading from SD card
    sd_clock_obj = SD.open(filename, FILE_READ);
    if (sd_clock_obj.readBytes(sd_clock_date, 11) != 11) {
        CRYO_DEBUG_MESSAGE("Failed to read clock date");
        return 0;
    }

    CRYO_DEBUG_MESSAGE("Skipping space");
    sd_clock_obj.read();
    
    CRYO_DEBUG_MESSAGE("Reading time from SD card");
    if (sd_clock_obj.readBytes(sd_clock_time, 8) != 8) {
        CRYO_DEBUG_MESSAGE("Failed to read clock time");
        return 0;
    }

    CRYO_DEBUG_MESSAGE("Closing file on SD card");
    sd_clock_obj.close();

    CRYO_DEBUG_MESSAGE("Converting file date and time to timestamp");
    PseudoRTC::get_time_from_compile_headers(sd_clock_date, sd_clock_time, sd_time);
    return 1;

}

uint8_t PseudoRTC::write_to_sd(const char* filename) {

    File sd_clock_obj;
    char timestamp_buffer[20];

    this->get_timestamp_compiler_format(timestamp_buffer);

    if (!SD.begin(SD_CHIP_SELECT)) {
        CRYO_DEBUG_MESSAGE("No SD card for writing RTC time");
        return 0;
    }

    CRYO_DEBUG_MESSAGE("Writing timestamp");
    // Try reading from SD card
    sd_clock_obj = SD.open(filename, O_READ | O_WRITE | O_CREAT); // don't append
    sd_clock_obj.seek(0);
    sd_clock_obj.write(timestamp_buffer);
    sd_clock_obj.close();
    CRYO_DEBUG_MESSAGE("Finished timestamp");
    
    return 1;

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
    digitalWrite(LED_BUILTIN, HIGH);
    cryo_rtc.tick();
    cryo_asleep_flag_debug = false;
    digitalWrite(LED_BUILTIN, LOW);

}

void cryo_rtc_sd_callback(uint16_t* date, uint16_t* time) {
    /* Reference (modified from)
        https://forum.arduino.cc/t/file-creation-date-and-time-in-sd-card/336037/5
    */

    // return date using FAT_DATE macro to format fields
    *date = FAT_DATE(cryo_rtc.year, cryo_rtc.month+1, cryo_rtc.day);

    // return time using FAT_TIME macro to format fields
    *time = FAT_TIME(cryo_rtc.hour, cryo_rtc.minute, cryo_rtc.second);
}
//--------------

PseudoRTC* cryo_get_rtc() {
    return &cryo_rtc;
}

