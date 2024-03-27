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