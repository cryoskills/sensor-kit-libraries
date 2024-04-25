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

#include "cryo_system.h"
#include "cryo_power.h"
#include "cryo_sleep.h"
#include "cryo_radio.h"
#include "RH_RF95.h"

RH_RF95 rf95(
    CRYO_PIN_RADIO_CS,
    CRYO_PIN_RADIO_IRQ
);

// Radio packet to use during sending
cryo_radio_packet radio_packet; 
PseudoRTC* radio_rtc;

uint8_t cryo_radio_init(uint32_t sensor_id, PseudoRTC* rtc) {
    
    // Attempt to start the RF95 radio module
    while (!rf95.init()) {
        CRYO_DEBUG_MESSAGE("LoRa radio init failed");
        return 0;
    }
    CRYO_DEBUG_MESSAGE("LoRa radio init OK!");
    
    // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
    if (!rf95.setFrequency(434.0)) {
        CRYO_DEBUG_MESSAGE("setFrequency failed");
        return 0;
    }
    CRYO_DEBUG_MESSAGE("Set Freq to 434 MHz"); 

    // you can set transmitter powers from 5 to 23 dBm:
    rf95.setTxPower(23, false);

    // Assign the radio_rtc pointer so we can access timestamps
    radio_rtc = rtc;

    // Initialise packet
    radio_packet.packet_type = CRYO_RADIO_PACKET_TYPE;
    radio_packet.packet_length = sizeof(cryo_radio_packet);
    radio_packet.packet_id = 0;
    // assign sensor ID from config
    radio_packet.sensor_id = sensor_id;
    radio_packet.ds18b20_temperature = 0;
    radio_packet.pt1000_temperature = 0;
    radio_packet.raw_adc_value = 0;
    radio_packet.battery_current = 0;
    radio_packet.battery_voltage = 0;
    radio_packet.solar_panel_current = 0;
    radio_packet.solar_panel_voltage = 0;
    radio_packet.load_voltage = 0;
    radio_packet.load_current = 0;
    rtc->get_timestamp(radio_packet.timestamp);

    return 1;

}

void cryo_radio_enable() {

    digitalWrite(CRYO_PIN_RADIO_ENABLE, HIGH);

}

void cryo_radio_disable() {

    digitalWrite(CRYO_PIN_RADIO_ENABLE, LOW);

}

int16_t packetnum = 0; 

int32_t cryo_radio_send_packet(float_t ds18b20_temp, float_t pt1000_temp)
{
    // Send packet with a fake raw value
    return cryo_radio_send_packet(ds18b20_temp, pt1000_temp, 0xffffffff);
}

int32_t cryo_radio_send_packet(
    float_t ds18b20_temp,
    float_t pt1000_temp,
    uint32_t raw_adc_value
) {

    CRYO_DEBUG_MESSAGE("Assigning user values to packet");
    Serial1.flush();
    // Copy data into the radio packet to send
    radio_packet.ds18b20_temperature = ds18b20_temp;
    radio_packet.pt1000_temperature = pt1000_temp;
    radio_packet.raw_adc_value = raw_adc_value;

    // Now assign housekeeping values
    CRYO_DEBUG_MESSAGE("Assigning housekeeping data to packet");
    Serial1.flush();
    radio_packet.battery_voltage = cryo_power_battery_voltage();
    radio_packet.battery_current = cryo_power_battery_current();
    radio_packet.solar_panel_voltage = cryo_power_solar_panel_voltage();
    radio_packet.solar_panel_current = cryo_power_solar_panel_current();
    radio_packet.load_voltage = cryo_power_load_voltage();
    radio_packet.load_current = cryo_power_load_current();

    CRYO_DEBUG_MESSAGE("Assigning timestamp to packet");
    Serial1.flush();
    // copy timestamp
    radio_rtc->get_timestamp(radio_packet.timestamp);

    CRYO_DEBUG_MESSAGE("enabling radio module");
    Serial1.flush();
    // Turn on radio modulke
    cryo_radio_enable();

    // Serial1.print("cryo_radio_packet is bytes long: ");
    // CRYO_DEBUG_MESSAGE(sizeof(radio_packet));
    Serial1.flush();

    CRYO_DEBUG_MESSAGE("Sending packet..."); delay(10) ;
    rf95.send((uint8_t *) &radio_packet, sizeof(radio_packet));
    CRYO_DEBUG_MESSAGE("Waiting for packet to complete..."); delay(10);
    rf95.waitPacketSent();

    // Increment the sequence id
    radio_packet.packet_id++;

    cryo_radio_disable();
    CRYO_DEBUG_MESSAGE("Disabling radio");
    
    // // Do something with the packet
    return sizeof(radio_packet);

}

int32_t cryo_radio_receive_packet(cryo_radio_packet* packet) {

    int32_t rssi = -999;
    return cryo_radio_receive_packet(packet, &rssi);

}

int32_t cryo_radio_receive_packet(cryo_radio_packet* packet, int32_t* rssi) {

    // if (rf95.available())
    // {   
    //     // Should be a message for us now   
    //     uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    //     uint8_t len = 6; // sizeof(cryotemp_packet); // sizeof(buf);
    //     if (rf95.recv(buf, &len)) {
    //         memcpy(packet, buf, len);
    //         return 1;
    //     } else {
    //         CRYO_DEBUG_MESSAGE("recv failed");
    //     }

    // }
    // return 0;

    if (rf95.available())
    {
        // CRYO_DEBUG_MESSAGE("Message available");
        // Should be a message for us now
        // uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
        uint8_t len = sizeof(*packet);
        if (rf95.recv((uint8_t*)packet, &len))
        {
            digitalWrite(LED_BUILTIN, HIGH);
            // CRYO_DEBUG_MESSAGE("Received packet");
            // RH_RF95::printBuffer("Received: ", buf, len);
            // Serial1.print("Got: ");
            // CRYO_DEBUG_MESSAGE((char*)buf);
            // Serial1.print("Packet millis(): ");
            // CRYO_DEBUG_MESSAGE(packet->raw_adc_value);
            // Serial1.print("RSSI: ");
            // CRYO_DEBUG_MESSAGE(rf95.lastRssi(), DEC);
            *rssi = rf95.lastRssi();
            digitalWrite(LED_BUILTIN, LOW);
            return 1;
        }
        else
        {
            CRYO_DEBUG_MESSAGE("Receive failed");
        }
    }

    return 0;

}