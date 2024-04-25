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

#include "Arduino.h"
#include "cryo_power.h"

// Initialise ina3221 object
INA3221 ina3221(INA3221_ADDR40_GND);

int32_t cryo_power_init() {

    ina3221.begin();
    ina3221.reset();

    // Set shunt resistors to 10 mOhm for all channels
    ina3221.setShuntRes(
        CRYO_POWER_SHUNT_RESISTOR, 
        CRYO_POWER_SHUNT_RESISTOR, 
        CRYO_POWER_SHUNT_RESISTOR
    );

    // Set series filter resistors to 10 Ohm for all channels.
    // Series filter resistors introduce error to the current measurement.
    // The error can be estimated and depends on the resitor values and the bus
    // voltage.
    ina3221.setFilterRes(
        CRYO_POWER_FILTER_RESISTOR, 
        CRYO_POWER_FILTER_RESISTOR, 
        CRYO_POWER_FILTER_RESISTOR
    );

    // Set monitoring mode to trigger
    // ina3221.setModeTriggered();

    // return true only if we can read from the IC correctly
    return (ina3221.getManufID() == 0x5449);
}

float_t cryo_power_battery_voltage() {
    return (float_t) ina3221.getVoltage(CRYO_POWER_BATTERY_CHANNEL);
}

float_t cryo_power_battery_current() {
    return (float_t) ina3221.getCurrent(CRYO_POWER_BATTERY_CHANNEL);
}

float_t cryo_power_solar_panel_voltage() {
    return (float_t) ina3221.getVoltage(CRYO_POWER_PANEL_CHANNEL);
}

float_t cryo_power_solar_panel_current() {
    return (float_t) ina3221.getCurrent(CRYO_POWER_PANEL_CHANNEL);
}

float_t cryo_power_load_voltage() {
    return (float_t) ina3221.getVoltage(CRYO_POWER_LOAD_CHANNEL);
}

float_t cryo_power_load_current() {
    return (float_t) ina3221.getCurrent(CRYO_POWER_LOAD_CHANNEL);
}