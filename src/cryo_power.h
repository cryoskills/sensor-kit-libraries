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
    cryo_power.h

DEPENDENCIES:
    [INA3221](https://github.com/Tinyu-Zhao/INA3221/tree/main/src)
        - requires modification of INA3221.cpp as below

            LINE 418 in INA3221::getShuntVoltage() should read
            
        ```
            res = (int32_t)(((int16_t)val_raw) / 8) * 40;
        ```

DESCRIPTION: 
    Wraps RFM95 in RadioHead to provide an interface to the RFM96W radio module.

    Provides send_packet, send_debug_packet and receive_packet methods.

CONFIGURATION:
    

EXAMPLE USAGE:
*/
#include "INA3221.h"

#define CRYO_POWER_SHUNT_RESISTOR 100 // mOhms
#define CRYO_POWER_FILTER_RESISTOR 10 // Ohms

#define CRYO_POWER_BATTERY_CHANNEL INA3221_CH1
#define CRYO_POWER_PANEL_CHANNEL INA3221_CH2
#define CRYO_POWER_LOAD_CHANNEL INA3221_CH3

int32_t cryo_power_init();

float_t cryo_power_battery_voltage();
float_t cryo_power_battery_current();
float_t cryo_power_solar_panel_voltage();
float_t cryo_power_solar_panel_current();
float_t cryo_power_load_voltage();
float_t cryo_power_load_current();