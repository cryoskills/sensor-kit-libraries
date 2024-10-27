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

PseudoRTC cryo_rtc;
volatile boolean cryo_asleep_flag_debug = false;
uint32_t cryo_meas_interval = 10;

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

    REG_RTC_READREQ |= 1; // start read request
    while (REG_RTC_STATUS & 0b10000000) {;} // wait for RTC to be ready

    uint32_t myDT   = REG_RTC_MODE2_CLOCK;
    this->year      = (myDT & RTC_MODE2_CLOCK_YEAR_Msk) >> RTC_MODE2_CLOCK_YEAR_Pos;
    this->month     = (myDT & RTC_MODE2_CLOCK_MONTH_Msk) >> RTC_MODE2_CLOCK_MONTH_Pos;
    this->day       = (myDT & RTC_MODE2_CLOCK_DAY_Msk) >> RTC_MODE2_CLOCK_DAY_Pos;
    this->hour      = (myDT & RTC_MODE2_CLOCK_HOUR_Msk) >> RTC_MODE2_CLOCK_HOUR_Pos;
    this->minute    = (myDT & RTC_MODE2_CLOCK_MINUTE_Msk) >> RTC_MODE2_CLOCK_MINUTE_Pos;
    this->second    = (myDT & RTC_MODE2_CLOCK_SECOND_Msk) >> RTC_MODE2_CLOCK_SECOND_Pos;

    return this->rtc_time;
    
}

void PseudoRTC::set_time(PseudoRTC::time time) {
    
    // Update RTC registers
    REG_RTC_MODE2_CLOCK = 
        RTC_MODE2_CLOCK_YEAR(time.year - 2000) // offset from ref 2000
    | RTC_MODE2_CLOCK_MONTH(time.month)
    | RTC_MODE2_CLOCK_DAY(time.day)
    | RTC_MODE2_CLOCK_HOUR(time.hour)
    | RTC_MODE2_CLOCK_MINUTE(time.minute)
    | RTC_MODE2_CLOCK_SECOND(time.second);
    
    while (REG_RTC_STATUS & 0b10000000) {;}

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

    CRYO_DEBUG_MESSAGE("Setting compiler date/time as: ");
    CRYO_DEBUG_MESSAGE(date);
    CRYO_DEBUG_MESSAGE(time);

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
    this->get_time();
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

void __configure_rtc_gclk(void) {
    
    /* Modified from https://github.com/IowaDave/SAMD21-RTC-Clock/ */
    // prescale GCLK4
    REG_GCLK_GENDIV = 
        GCLK_GENDIV_ID(0x04)
    | GCLK_GENDIV_DIV(8);
    while (GCLK->STATUS.bit.SYNCBUSY) {;}
    // select external 32K crystal as source
    REG_GCLK_GENCTRL =
        GCLK_GENCTRL_ID(0x04)
    | GCLK_GENCTRL_SRC_XOSC32K 
    | GCLK_GENCTRL_DIVSEL
    | GCLK_GENCTRL_RUNSTDBY;
    while (GCLK->STATUS.bit.SYNCBUSY) {;}
    GCLK->GENCTRL.bit.GENEN = 1; // enable the generator
    while (GCLK->STATUS.bit.SYNCBUSY) {;}
    // route output to the RTC peripheral
    GCLK->CLKCTRL.reg = 
        GCLK_CLKCTRL_ID_RTC
    | GCLK_CLKCTRL_GEN_GCLK4; // component/gclk.h line 202
    while (GCLK->STATUS.bit.SYNCBUSY) {;}
    GCLK->CLKCTRL.bit.CLKEN = 1; // enable the GCLK
    while (GCLK->STATUS.bit.SYNCBUSY) {;}

}

void __configure_evsys_gclk(void) {

    REG_GCLK_GENDIV = 
    GCLK_GENDIV_ID(0x07)
    | GCLK_GENDIV_DIV(0);
    while (GCLK->STATUS.bit.SYNCBUSY) {;}
    // select external 32K crystal as source
    REG_GCLK_GENCTRL =
        GCLK_GENCTRL_ID(0x07)
    | GCLK_GENCTRL_SRC_XOSC32K
    | GCLK_GENCTRL_DIVSEL
    | GCLK_GENCTRL_RUNSTDBY;
    while (GCLK->STATUS.bit.SYNCBUSY) {;}
    GCLK->GENCTRL.bit.GENEN = 1; // enable the generator
    while (GCLK->STATUS.bit.SYNCBUSY) {;}
    GCLK->CLKCTRL.reg = 
        GCLK_CLKCTRL_ID_EVSYS_0
    | GCLK_CLKCTRL_GEN_GCLK7; // component/gclk.h line 202
    while (GCLK->STATUS.bit.SYNCBUSY) {;}
    GCLK->CLKCTRL.bit.CLKEN = 1; // enable the GCLK
    while (GCLK->STATUS.bit.SYNCBUSY) {;}

}

void cryo_configure_clock(const char* date, const char* time, uint32_t meas_interval) {
    
    CRYO_DEBUG_MESSAGE("Initialising RTC clocks");
    
    /* Modified from https://github.com/IowaDave/SAMD21-RTC-Clock/ */
    
    // Configure RTC GCLK
    // ----------------------------------------------------------------
    PM->APBCMASK.reg |= PM_APBCMASK_EVSYS;
    __configure_rtc_gclk();

    // apply power to the RTC
    PM->APBASEL.bit.APBADIV = 0; // don't prescale the PM clock
    PM->APBAMASK.bit.RTC_ = 1; // unmask the RTC

    // Enable the external 32-bit oscillator in standby
    SYSCTRL->XOSC32K.bit.RUNSTDBY = 1;
    SYSCTRL->XOSC32K.bit.STARTUP = 6;

    // Configure RTC
    // ----------------------------------------------------------------
    RTC->MODE2.CTRL.bit.ENABLE = 0;         // Enable the RTC
    while (RTC->MODE2.STATUS.bit.SYNCBUSY); // Wait for synchronization

    RTC->MODE2.CTRL.bit.SWRST = 1;          // Software reset the RTC
    while (RTC->MODE2.STATUS.bit.SYNCBUSY); // Wait for 
    
    REG_RTC_MODE2_CTRL = 
        RTC_MODE2_CTRL_MODE(RTC_MODE2_CTRL_MODE_CLOCK_Val)
    | RTC_MODE2_CTRL_PRESCALER(RTC_MODE2_CTRL_PRESCALER_DIV64_Val);

    RTC->MODE2.Mode2Alarm->MASK.bit.SEL = 0x03; // HHMMSS

    while (REG_RTC_STATUS & 0b10000000) {;} 
    
    __configure_evsys_gclk();

    // Configure Event System
    // ----------------------------------------------------------------
    EVSYS->USER.bit.CHANNEL = EVSYS_USER_CHANNEL_0;

    EVSYS->CHANNEL.reg = EVSYS_CHANNEL_EDGSEL_RISING_EDGE |                  // Rising event edge detection
                        //EVSYS_CHANNEL_PATH_SYNCHRONOUS |                 // Set event path as synchronous
                        EVSYS_CHANNEL_PATH_RESYNCHRONIZED |                 // Set event path as resynchronized
                        EVSYS_CHANNEL_EVGEN(EVSYS_ID_GEN_RTC_PER_5) |       // Set event generator (sender) as compare channel 0
                        EVSYS_CHANNEL_CHANNEL(0);                           // Attach the generator (sender) to channel 0

    // // enable event interrupts
    NVIC_SetPriority(EVSYS_IRQn, 0);
    NVIC_EnableIRQ(EVSYS_IRQn);
    EVSYS->INTENSET.bit.EVD0 = 1;
    
    // // enable RTC interrupts
    NVIC_SetPriority(RTC_IRQn, 0);
    NVIC_EnableIRQ(RTC_IRQn);
    RTC->MODE2.INTENSET.bit.ALARM0 = 1;

    // Configure RTC alarms
    // ----------------------------------------------------------------
    
    // Configure RTC prescaler interrupts
    RTC->MODE2.EVCTRL.bit.PEREO5 = 1;
    while (REG_RTC_STATUS & 0b10000000) {;}

    REG_RTC_MODE2_CTRL |= (RTC_MODE2_CTRL_ENABLE); // enable RTC
    while (REG_RTC_STATUS & 0b10000000) {;}

    // Set measurement interval
    cryo_meas_interval = meas_interval;
    
    // Configure deep sleep
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
    
    cryo_rtc.set_time_from_compile_headers(date, time);

}

void cryo_wakeup() {

    #ifdef CRYO_SLEEP_MODE_DEBUG
        cryo_wakeup_debug()
    #else

        // zpmCPUClk48M();
        // Removed 48M clock as this appeared to be causing the device to hang
        // but stable now on transmitter

    #endif

}
void cryo_wakeup_debug() {}; // do nothing

void cryo_raise_alarms() {
    
    // check alarms
    cryo_rtc.raise_alarms();
    
}

void cryo_reset_alarm() {

    PseudoRTC::time time = cryo_get_rtc()->get_time();

    uint8_t seconds, minutes, hours = 0;
    uint8_t d_seconds, d_minutes, d_hours = 0;

    d_hours = (cryo_meas_interval / 60 / 60);
    d_minutes = (cryo_meas_interval - d_hours * 60 * 60) / 60;
    d_seconds = cryo_meas_interval - d_hours * 60 * 60 - d_minutes * 60;

    seconds = time.second + d_seconds;
    minutes = time.minute + d_minutes;
    hours = time.hour + d_hours;

    if (seconds >= 60) {
        minutes += 1;
        seconds = seconds - 60;
    }

    if (minutes >= 60) {
        hours += 1;
        minutes = minutes - 60;
    }

    hours = hours % 24;

    RTC->MODE2.Mode2Alarm->ALARM.bit.HOUR = hours;
    RTC->MODE2.Mode2Alarm->ALARM.bit.MINUTE = minutes;
    RTC->MODE2.Mode2Alarm->ALARM.bit.SECOND = seconds;
    while (REG_RTC_STATUS & 0b10000000) {;}
    
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
        // zpmCPUClk32K();
        // zpmSleep();
        // SCB->SCR |= SCB_SCR_SLEEPONEXIT_Msk;
        __DSB();
        __WFI();
        SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
    
    #endif

}
void cryo_sleep_debug() {
    cryo_asleep_flag_debug = true;
    while (cryo_asleep_flag_debug) {}; 
}; // do nothing

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

