#include "cryo_system.h"

/* ---------------- GLOBAL VARIABLES ---------------- */
CRYO_DEBUG_LEVEL CRYO_DEBUG = CRYO_DEBUG_LEVEL::DISABLED;
File cryo_debug_file;
char rtc_timestamp[CRYO_RTC_TIMESTAMP_LENGTH];

/* ---------------- FUNCTION DEFINITIONS ---------------- */

void cryo_error(uint8_t error_code) {

    // Flash out the error code
    pinMode(LED_BUILTIN, OUTPUT);
    // Pseudo morse encoded - long dash is 1, short dash is 0
    while (1) {
        for (uint8_t k = 0; k < 8; k++) {
            digitalWrite(LED_BUILTIN, LOW);
            delay(50);
            digitalWrite(LED_BUILTIN, HIGH);
            if ((error_code >> (7-k)) & 1) {
                delay(ERROR_CODE_DELAY * 4);
            } else {
                delay(ERROR_CODE_DELAY);
                digitalWrite(LED_BUILTIN, LOW);
                delay(ERROR_CODE_DELAY * 3);
            }
            digitalWrite(LED_BUILTIN, LOW);
            delay(ERROR_CODE_DELAY * 4);
        }
        delay(ERROR_CODE_DELAY * 4);
    }

}

void _cryo_debug_message(const char* message) {

    if (CRYO_DEBUG & CRYO_DEBUG_LEVEL::DEBUG_SERIAL) {
        _cryo_debug_message_serial(message);
    }

    // If SD is enabled, then do that
    if (CRYO_DEBUG & CRYO_DEBUG_LEVEL::DEBUG_SD) {
        _cryo_debug_message_sd(message);
    }
}

void _cryo_debug_message_serial(const char* message) {

    Serial.println(message);
    SerialDebug.println(message);
    // For now, only flush the Debug port as that is always open (i.e. not blocking)
    SerialDebug.flush();

}

void _cryo_debug_sd_open() {
    
    if (!SD.begin(SD_CHIP_SELECT)) {
        // use raw _cryo_debug_message_serial here to avoid recursion issues
        _cryo_debug_message_serial("Failed to init SD card.");
        cryo_error(CRYO_ERROR_SD_INIT);
    };

    // Open SD card
    cryo_debug_file = SD.open(CRYO_DEBUG_SD_FILENAME, FILE_WRITE);

}

void _cryo_debug_sd_close() {
    cryo_debug_file.close();
}

void _cryo_debug_sd_flush() {
    cryo_debug_file.flush();
}

void _cryo_debug_message_sd(const char* message) {
    
    PseudoRTC* local_rtc;
    local_rtc = cryo_get_rtc();
    local_rtc->get_timestamp(rtc_timestamp);

    _cryo_debug_sd_open();
    // Print timestamp
    cryo_debug_file.write(rtc_timestamp);
    cryo_debug_file.write(" : ");
    // Write message
    cryo_debug_file.write(message);
    cryo_debug_file.write("\n");
    _cryo_debug_sd_close();

}

