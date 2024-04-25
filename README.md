# CryoSkills Datalogger - Libraries
This repository contains libraries for the CryoSkills datalogger which are described below.

For the most part, these libraries provide 'wrapping' functions around existing functionality to make the firmware easier to follow.

## Library - `cryo_system`
The `cryo_system` library implements debugging and error functionality for the CryoSkills datalogger.

### Debug Messages
Modifying the `CRYO_DEBUG` level in the datalogger firmware sets whether debug messages (sent using `CRYO_DEBUG_MESSAGE`) are disabled, written to the USB Debug port and/or written to the SD card.

| Debug Mode          | Description                                                      |
| ------------------- | ---------------------------------------------------------------- |
| DISABLED            | Debug messages disabled.                                         | 
| DEBUG_SERIAL        | Debug messages forwarded to the USB debug port.                  |
| DEBUG_SD            | Debug messages written to `DEBUG.TXT` on the SD card.            |
| DEBUG_SERIAL_AND_SD | Debug messages forward to USB debug port and written to SD card. |  

You can implement additional debug messages in the code using the `CRYO_DEBUG_MESSAGE` function as below:

```
CRYO_DEBUG_MESSAGE("Something happened!");
```

### Error Messages
The `cyro_error` function is used to help identify the cause of crashes in the firmware by blinking a 'morse-style' binary code.  All error messages are 8-bits long and consist of ;longer blinks of 400ms (corresponding to a binary 1) and
shorter blinks (corresponding to a binary 0).

Currently implemented error codes are listed below, however custom error codes can be added by calling `cryo_error` with the appropriate numeric value.

| Error Code                | Value      | Description |
| ------------------------- | ---------- | ----------- |
| CRYO_ERROR_RADIO_INIT     | 0b10000000 | The radio module has failed to initialise - is the datalogger board powered (and not just the Adalogger)? |
| CRYO_ERROR_DS18B20_INIT   | 0b01000000 | No DS18B20 sensor could be found. |
| CRYO_ERROR_SD_INIT        | 0b00100000 | The SD card failed to initialise - is there an SD card present? Is it formatted to FAT32? |
| CRYO_ERROR_INA3221_INIT   | 0b00010000 | The INA3221 module failed to initialise - is the datalogger board powered (and not just the Adalogger)? |

## Library - `cryo_sleep`
The `cryo_sleep` library keeps track of the time (using the integrated real-time clock) and conserves power by setting the microcontroller to a low-power sleep module between measurements.

The software pattern for programs using the `cryo_sleep` library is shown below

```
#include "cryo_sleep.h"

void my_function();

void setup() {
    // Do setup activities
    SerialDebug.begin(9600);
    // ...

    // Configure the real-time clock
    cryo_configure_clock(__DATE__, __TIME__);
    
    // Configure a function to be called every 30 seconds 
    cryo_add_alarm_every(30, my_function);
}

void loop() {
    // Wakeup and reconfigure the clock speed
    cryo_wakeup();
    // Run any functions that need to be called
    cryo_raise_alarms();
    // Go back to sleep!
    cryo_sleep();
}

void my_function() {
    SerialDebug.println("Hello, world!");
}
```

## Library - `cryo_adc`
The `cryo_adc` library configures the analogue-to-digital converter (ADC) in the SAMD21 microcontroller to be used in its 'differential input' mode.  This allows for improved sensitivity and precision when using the PT1000 temperature sensor through gain and averaging.

## Library - `cryo_radio`
The `cryo_radio` library controls the RFM96W radio module on the datalogger PCB to send temperature data and housekeeping information on a 433 MHz LoRa radio link.

## Library - `cryo_power`
The `cryo_power` library uses the integrated INA3221 power meter on the datalogger PCB to give us information about the power consumption of different components of the sensor kit (solar panel, battery, circuit board). This is useful for debugging and monitoring the battery level.

# Requirements
The CryoSkills datalogger libraries depend on the following third-party libraries:

| Library Name     | Description                                       | URL                                                |
| ---------------- | ------------------------------------------------- | -------------------------------------------------- |
| CryoSkills       | Auxiliary libraries for the CryoSkills datalogger | https://github.com/cryoskills/sensor-kit-libraries |
| INA3221          | CryoSkills modification of INA3221 library        | https://github.com/cryoskills/INA3221              |
| ZeroPowerManager | Required to compile CryoSkills library            | https://github.com/ee-quipment/ZeroPowerManager    |
| RadioHead        | Required to control the RFM96W radio module       | https://github.com/PaulStoffregen/RadioHead/       | 

