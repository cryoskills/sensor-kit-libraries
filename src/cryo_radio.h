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
#ifndef CRYO_RADIO
#define CRYO_RADIO

#include <Arduino.h>
#include "cryo_sleep.h"

#define CRYO_RADIO_ENABLE_PIN 6
#define CRYO_RADIO_IRQ_PIN 9
#define CRYO_RADIO_CS_PIN 10

#define CRYO_RADIO_ERROR_FAILED_INIT 0

#define CRYO_RADIO_PACKET_TYPE 0xC5

// Define simplest radio packet
typedef struct cryo_radio_packet {
    uint8_t packet_type;            // ignore - set to 0
    uint8_t packet_length;          // set to 70 - standard packet length
    uint32_t packet_id;              // 
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


int32_t cryo_radio_init(uint32_t sensor_id, PseudoRTC* rtc);
void cryo_radio_enable();
void cryo_radio_disable();

int32_t cryo_radio_send_packet(float_t ds18b20_temp, float_t pt1000_temp);
int32_t cryo_radio_send_packet(float_t ds18b20_temp, float_t pt1000_temp, uint32_t raw_adc_value);

int32_t cryo_radio_receive_packet(cryo_radio_packet* packet);
int32_t cryo_radio_receive_packet(cryo_radio_packet* packet, int32_t* rssi);

#endif