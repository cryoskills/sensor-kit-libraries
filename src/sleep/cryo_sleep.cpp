#include "cryo_sleep.h"
#include "ZeroPowerManager.h"

extern PseudoRTC cryo_rtc;

uint16_t PseudoRTC::month_from_str(const char* year_str) {
    for (int j = 0; j < 48; j += 4) {
        if (!strcmp(PseudoRTC::NAMES_OF_MONTH + j, year_str)) {
            return (j/4);
        }
    }
    return 0xffff;
}

void PseudoRTC::tick() {

    this->second += 1;
    // Increment minute
    if (this->second > 59) {
        this->second = 0;
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
    if (this->month == 1 && PseudoRTC::is_leap_year(this->rtc_time) && this->day > 28) {
        this->month += 1;
        this->day = 0;
    // not nice, but make implicit correction for -1 index here so that 
    // the days_of_month array is logical
    } else if (this->day > PseudoRTC::DAYS_OF_MONTH[this->month] - 1) {
        this->month += 1;
        this->day = 0;
    }

    // Increment year
    if (this->month > 11) {
        this->year += 1;
        this->month = 0;
    }

    // Reset year at millenia indices - eek!
    if (this->year > 999) {
        this->year = 0;
    };

}

PseudoRTC::time PseudoRTC::get_time () {
    return this->rtc_time;
}

void PseudoRTC::set_time(PseudoRTC::time time) {
    this->rtc_time = time;
}

void PseudoRTC::set_time_from_compile_headers() {

    char month_buffer[4];
    PseudoRTC::time new_time;
    // Read date
    sscanf(__DATE__, "%s %d %d", month_buffer, &new_time.day, &new_time.year);
    new_time.month = PseudoRTC::month_from_str(month_buffer);
    // Read time
    sscanf(__TIME__, "%d:%d:%ud", &new_time.hour, &new_time.minute, &new_time.second);

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

void cryo_configure_clock() {
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
    cryo_rtc.set_time_from_compile_headers();
    zpmRTCInterruptEvery(1024, cryo_rtc_handler);
}

void cryo_wakeup() {

  zpmCPUClk48M();

}

void cryo_sleep() {
  
    SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;	
	
    zpmSleep();

    SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;

}

void cryo_rtc_handler() {
    
    // Perform RTC tick
    cryo_rtc.tick();

}