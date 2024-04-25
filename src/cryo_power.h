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
    [INA3221](https://github.com/cryoskills/INA3221)
    Modified to enable reading negative currents

DESCRIPTION: 
    Provides wrappers functions for initialising the INA3221 and reading power
    and current measurements from the battery, solar panel and circuit.

CONFIGURATION:
    

EXAMPLE USAGE:
*/
#include <Arduino.h>
#include "INA3221.h"

#ifndef CYRO_POWER_H
#define CRYO_POWER_H

#define CRYO_POWER_SHUNT_RESISTOR 100 // mOhms
#define CRYO_POWER_FILTER_RESISTOR 10 // Ohms

#define CRYO_POWER_BATTERY_CHANNEL INA3221_CH1
#define CRYO_POWER_PANEL_CHANNEL INA3221_CH2
#define CRYO_POWER_LOAD_CHANNEL INA3221_CH3

/*
    name:           cryo_power_init()
    description:    initialises the INA3221 power monitor circuitry and I2C interface.
    arguments:      none
    returns:        1 
                        - if the I2C bus is initialised correctly and data can be read from the INA3221
                    0   
                        - if the initialising fails 

*/
int32_t cryo_power_init();

/* 
    name:           cryo_power_battery_voltage()
    description:    returns the shunt voltage on the low side of the battery shunt resistor (R2)
    arguments:      none
    returns:        floating point (float_t) voltage in Volts 
*/
float_t cryo_power_battery_voltage();

/* 
    name:           cryo_power_solar_panel_voltage()
    description:    returns the shunt voltage on the low side of the solar panel shunt resistor (R1)
    arguments:      none
    returns:        floating point (float_t) voltage in Volts 
*/
float_t cryo_power_solar_panel_voltage();

/* 
    name:           cryo_power_load_voltage()
    description:    returns the shunt voltage on the low side of the load shunt resistor (R3)
    arguments:      none
    returns:        floating point (float_t) voltage in Volts 
*/
float_t cryo_power_load_voltage();

/* 
    name:           cryo_power_battery_current()
    description:    returns the shunt current on the battery shunt resistor (R2)
    arguments:      none
    returns:        floating point (float_t) current in Amperes 
*/
float_t cryo_power_battery_current();

/* 
    name:           cryo_power_solar_panel_current()
    description:    returns the shunt current on the solar panel shunt resistor (R1)
    arguments:      none
    returns:        floating point (float_t) current in Amperes 
*/
float_t cryo_power_solar_panel_current();

/* 
    name:           cryo_power_load_current()
    description:    returns the shunt current on the load shunt resistor (R3)
    arguments:      none
    returns:        floating point (float_t) current in Amperes 
*/
float_t cryo_power_load_current();

#endif