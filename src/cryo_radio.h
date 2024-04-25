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
    cryo_radio.h

DEPENDENCIES:
    RadioHead - http://www.airspayce.com/mikem/arduino/RadioHead/
    cryo_sleep.h - for RTC

DESCRIPTION: 
    Wraps RFM95 in RadioHead to provide an interface to the RFM96W radio module.

    Provides send_packet, send_debug_packet and receive_packet methods.

CONFIGURATION:
    

EXAMPLE USAGE:
*/

#include <Arduino.h>
#include "cryo_sleep.h"

#ifndef CRYO_RADIO_H
#define CRYO_RADIO_H

/* 
    Radio Pin Declarations
    ----------------------
    These can be defined prior to included cyro_radio.h to 
    configure alternative radio pins for the RFM96 radio module.
*/
#ifndef CRYO_PIN_RADIO_ENABLE
#define CRYO_PIN_RADIO_ENABLE 6
#endif 
//
#ifndef CRYO_PIN_RADIO_IRQ
#define CRYO_PIN_RADIO_IRQ 9
#endif 
//
#ifndef CRYO_PIN_RADIO_CS
#define CRYO_PIN_RADIO_CS 10
#endif 

/* 
    Radio Packet Type
    -----------------
    This serves as a placeholder in case there is a desire in later
    versions to update the radio drivers so that multiple packet
    types can be sent and - critically - identified at ther receiver
    to be properly decoded.

    For now, all packets should use the default CRYO_RADIO_PACKET_TYPE.
*/
#define CRYO_RADIO_PACKET_TYPE 0xC5

/*
    Radio Packet Structure
    ----------------------
    The radio packet structure is given by the C-type struct below.
*/
typedef struct cryo_radio_packet {
    uint8_t packet_type;            
    uint8_t packet_length;         
    uint32_t packet_id;
    uint32_t sensor_id;
    float_t ds18b20_temperature;
    float_t pt1000_temperature;
    uint32_t raw_adc_value;
    float_t battery_voltage;
    float_t battery_current;
    float_t solar_panel_voltage;
    float_t solar_panel_current;
    float_t load_voltage;
    float_t load_current;
    char timestamp[CRYO_RTC_TIMESTAMP_LENGTH];
} cryo_radio_packet;

/*
    name:           cryo_radio_init(uint32_t sensor_id, PseudoRTC* rtc)
    description:    Initialises the RFM96 radio module and packet structure 
                    with default values.
    arguments:
                    uint32_t sensor_id
                        - 32-bit integer to serve as a unique identifier
                          for this instance of the datalogger
                    PseudoRTC* rtc
                        - pointer to an instance of PseudoRTC, used to 
                          retrieve current timestamp when transmitting
    returns:        
                    uint8_t
                        1 - returned if initialisation of radio was successful
                        0 - returned if initialisation of radio was unsuccessful
*/
uint8_t cryo_radio_init(uint32_t sensor_id, PseudoRTC* rtc);

/*
    name:           cryo_radio_enable()
    description:    pulls the pin defined by CRYO_PIN_RADIO_ENABLE high to switch
                    on the RFM96 module.
    arguments:      none
    returns:        none
*/
void cryo_radio_enable();

/*
    name:           cryo_radio_disable()
    description:    pulls the pin defined by CRYO_PIN_RADIO_ENABLE low to switch
                    off the RFM96 module.
    arguments:      none
    returns:        none
*/
void cryo_radio_disable();

/*
    name:           cryo_radio_send_packet(...)
    description:    sends a cryo_radio packet using the RFM96 radio module with
                    the temperature data, optional ADC data and housekeeping
                    information.
    arguments:      
                    float_t ds18b20_temp
                    - temperature in degrees Celcius read by the DS18B20 
                      digital temperature sensor

                    float_t pt1000_temp
                    - temperature in degrees Celcius as converted from the 
                      PT1000 digital temperature sensor

                    [optional]
                    uint32_t raw_adc_value
                    - a 32-bit field to store the raw ADC value.  In reality,
                       the ADC conversion should return a signed/unsigned 16-bit field
                       so appropriate knowledge on the receiver end is required 
                       to correctly interpret the value.
                       
    returns:        returns the size of the transmitted packet
*/
int32_t cryo_radio_send_packet(float_t ds18b20_temp, float_t pt1000_temp);
int32_t cryo_radio_send_packet(float_t ds18b20_temp, float_t pt1000_temp, uint32_t raw_adc_value);

int32_t cryo_radio_receive_packet(cryo_radio_packet* packet);
int32_t cryo_radio_receive_packet(cryo_radio_packet* packet, int32_t* rssi);

#endif