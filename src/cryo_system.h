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
#include "SPI.h"
#include "SD.h" 

#include "cryo_sleep.h"

#ifndef CRYO_SYSTEM_H
#define CRYO_SYSTEM_H

#define ERROR_CODE_DELAY 100
#define SerialDebug Serial1

/*
    SD Card Debugging Interval
    --------------------------
    Period at which to flush the output to the SD card debug file
*/
#define CRYO_DEBUG_SD_FLUSH 30 // seconds
/*
    SD Card Debug Log Name
    ----------------------
    Filename to write debug output to SD card to.
*/
#define CRYO_DEBUG_SD_FILENAME "/DEBUG.TXT"

#define SD_CHIP_SELECT 4

/* ---------------- ERROR CODES ---------------- */
#define CRYO_ERROR_RADIO_INIT       0b10000000
#define CRYO_ERROR_DS18B20_INIT     0b01000000
#define CRYO_ERROR_SD_INIT          0b00100000
#define CRYO_ERROR_SD_FILENAME      0b00100001
#define CRYO_ERROR_INA3221_INIT     0b00010000

/* ---------------- TYPE DEFINTIONS ---------------- */
typedef enum CRYO_DEBUG_LEVEL{
    DISABLED                = 0b00,
    DEBUG_SERIAL            = 0b01,
    DEBUG_SD                = 0b10,
    DEBUG_SERIAL_AND_SD     = 0b11
};

/* ---------------- FUNCTION DEFINITIONS ---------------- */
void cryo_error(uint8_t error_code);
void _cryo_debug_message(const char* message);
void _cryo_debug_message_serial(const char* message);
void _cryo_debug_message_sd(const char* message);
void _cryo_debug_sd_flush();
void _cryo_debug_sd_open();
void _cryo_deubg_sd_close();

#define CRYO_DEBUG_MESSAGE(msg) _cryo_debug_message(msg);

#endif