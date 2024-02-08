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

FILE: 
    cryo_sleep.h

DEPENDENCIES:
    zeroPowerManager - https://github.com/ee-quipment/ZeroPowerManager

DESCRIPTION: 
    Extends zeroPowerManager library to implement a:

        1.) wakeup
        2.) deal with alarms
        3.) sleep

    design pattern.  
    
    Includes a PseudoRTC class to provide
    date and time information, which can be configured manually or 
    updated from the compile headers __DATE__ and __TIME__.

CONFIGURATION:
    MAX_RTC_ALARMS 
        description:    the maximum number of RTC alarms to be allowed
        default value:  4 
        max value:      127

EXAMPLE USAGE:

    // Example calls "my_alarm_function" every 30 seconds

    #include <Arduino.h>
    #include "cryo_sleep.h"

    void my_alarm_function();

    void setup() {

        Serial.begin(9600);

        cryo_configure_clock();
        cryo_add_alarm_every(30, my_alarm_function);

    }

    void loop() {
        cryo_wakeup();
        cryo_raise_alarms();
        cryo_sleep();
    }

******************************************************************************/


#include <Arduino.h>

#ifndef __CRYO_SLEEP_H
#define __CRYO_SLEEP_H

#define MAX_RTC_ALARMS 4

// define PseudoRTC class so we can return it from cryo_ functions
class PseudoRTC;

/*
    name:           cryo_get_rtc()
    description:    returns the PseudoRTC object being used by the cryo_sleep library
    example:

                    PseudoRTC my_rtc = cryo_get_rtc();
                    Serial.printf(
                        "It's current %u seconds into the %u-th minute\n\r",
                        my_rtc.second,
                        my_rtc.minute
                    );

    arguments:      none
    returns:        PseudoRTC object
*/
PseudoRTC cryo_get_rtc();

/*
    name:           cryo_configure_clock()
    description:    initialises the SAMD21 RTC, sets the PseudoRTC time to match the compile headers
                    and configures the 1s interrupt to update the RTC
                    Should be called in setup()
    arguments:      none
    returns:        none
*/
void cryo_configure_clock();

/*
    name:           cryo_wakeup()
    description:    resets the Adalogger clock to 48MHz and other wakeup functions. 
                    Should be called at the start of loop()
    arguments:      none
    returns:        none
*/
void cryo_wakeup();

/*
    name:           cryo_raise_alarms()
    descriptions:   checks whether any of the outstanding RTC alarms have been activated
                    if an alarm flag is raised, the alarm callback function will be called.
                    Should be called during loop()
    arguments:      none
    returns:        none
*/
void cryo_raise_alarms();

/*
    name:           cryo_sleep()
    description:    configures the Adalogger for sleep mode then activates sleep mode.
                    Should be called at the end of loop()
    arguments:      none
    returns:        none
*/
void cryo_sleep();

/*
    name:           cryo_add_alarm_every(uint32_t seconds, void (*callback)())
    description:    adds an alarm function, defined by the function callback, to be called at an interval 'seconds'
    example:        
                    void my_alarm_function() {
                        Serial.println("Hello");
                    }
                    
                    void setup() {
                        cryo_add_alarm_every(1, my_alarm_function)
                    }

    arguments:      uint32_t seconds    - time in seconds after which to call this alarm,
                    void (*callback)()  - pointer to the callback function to be associated with this alarm
    returns:        none
*/
void cryo_add_alarm_every(uint32_t seconds, void (*callback)());

/*
    name:           cryo_rtc_handler()
    description:    called every second to update the real-time clock, shouldn't need to be used
    arguments:      none
    returns:        none
    
*/
void cryo_rtc_handler();

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

        // Constructor
        PseudoRTC();

        // tick() function is called once a second and updates the time
        void tick();

        // adds an alarm function (callback) to be called every 'interval' seconds
        // returns the alarm_id that has been assigned
        uint8_t add_alarm_every_n_seconds(uint32_t interval, void (*callback)());
        // removes the alarm assigned at alarm_id 
        void remove_alarm(uint8_t alarm_id);

        // returns the current time held in the PseudoRTC
        PseudoRTC::time get_time();
        // sets the time held in the PseudoRTC
        void set_time(PseudoRTC::time time);
        // updates the time in the PseudoRTC from __DATE__ and __TIME__ compile strings
        void set_time_from_compile_headers();

        // checks whether any alarm flags have been raised and, if so, calls them
        void raise_alarms();

    private:
    
        // Alarms
        // Functions:
        void check_alarms();
        // Variables:
        uint32_t alarm_intervals[MAX_RTC_ALARMS];
        uint32_t alarm_counts[MAX_RTC_ALARMS];
        uint8_t alarm_flags[MAX_RTC_ALARMS];
        void (*alarm_callback[MAX_RTC_ALARMS])();

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
        static bool is_leap_year(PseudoRTC::time time);

};

#endif

