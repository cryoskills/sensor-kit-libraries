#include "cryo_power.h"
#include "cryo_sleep.h"
#include "cryo_radio.h"
#include "RH_RF95.h"

RH_RF95 rf95(
    CRYO_RADIO_CS_PIN,
    CRYO_RADIO_IRQ_PIN
);

int32_t cryo_radio_init(uint32_t sensor_id, PseudoRTC* rtc) {
    
    // Attempt to start the RF95 radio module
    while (!rf95.init()) {
        Serial1.println("LoRa radio init failed");
        return 0;
    }
    Serial1.println("LoRa radio init OK!");
    
    // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
    if (!rf95.setFrequency(434.0)) {
        Serial1.println("setFrequency failed");
        return 0;
    }
    Serial1.print("Set Freq to: "); Serial1.println(434.0);

    // you can set transmitter powers from 5 to 23 dBm:
    rf95.setTxPower(23, false);

    // Assign the radio_rtc pointer so we can access timestamps
    radio_rtc = rtc;

    // Initialise packet
    radio_packet.packet_type = 0;
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
    // we don't initialise the timestamp here
    rtc->get_timestamp(radio_packet.timestamp);

    return 1;

}

void cryo_radio_enable() {

    digitalWrite(CRYO_RADIO_ENABLE_PIN, HIGH);

}

void cryo_radio_disable() {

    digitalWrite(CRYO_RADIO_ENABLE_PIN, LOW);

}

int16_t packetnum = 0; 

int32_t cryo_radio_send_packet(float_t ds18b20_temp, float_t pt1000_temp)
{
    // Send packet with a fake raw value
    cryo_radio_send_packet(ds18b20_temp, pt1000_temp, 0xffffffff);
}

int32_t cryo_radio_send_packet(
    float_t ds18b20_temp,
    float_t pt1000_temp,
    uint32_t raw_adc_value
) {

    // Copy data into the radio packet to send
    radio_packet.ds18b20_temperature = ds18b20_temp;
    radio_packet.pt1000_temperature = pt1000_temp;
    radio_packet.raw_adc_value = raw_adc_value;

    // Now assign housekeeping values
    radio_packet.battery_voltage = cryo_power_battery_voltage();
    radio_packet.battery_current = cryo_power_battery_current();
    radio_packet.solar_panel_voltage = cryo_power_solar_panel_voltage();
    radio_packet.solar_panel_current = cryo_power_solar_panel_current();
    radio_packet.load_voltage = cryo_power_load_voltage();
    radio_packet.load_current = cryo_power_load_current();

    // copy timestamp
    radio_rtc->get_timestamp(radio_packet.timestamp);

    // Turn on radio modulke
    cryo_radio_enable();

    Serial1.print("cryo_radio_packet is bytes long: ");
    Serial1.println(sizeof(radio_packet));

    Serial1.println("Sending..."); delay(10) ;
    rf95.send((uint8_t *) &radio_packet, sizeof(radio_packet));
    Serial1.println("Waiting for packet to complete..."); delay(10);
    rf95.waitPacketSent();

    // Increment the sequence id
    radio_packet.packet_id++;

    cryo_radio_disable();
    Serial1.println("Disabling radio");
    
    // // Do something with the packet
    return sizeof(radio_packet);

}

int32_t cryo_radio_receive_packet(cryo_radio_packet* packet) {

    // if (rf95.available())
    // {   
    //     // Should be a message for us now   
    //     uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    //     uint8_t len = 6; // sizeof(cryotemp_packet); // sizeof(buf);
    //     if (rf95.recv(buf, &len)) {
    //         memcpy(packet, buf, len);
    //         return 1;
    //     } else {
    //         Serial1.println("recv failed");
    //     }

    // }
    // return 0;

    if (rf95.available())
    {
        // Should be a message for us now
        // uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
        uint8_t len = sizeof(*packet);
        if (rf95.recv((uint8_t*)packet, &len))
        {
            digitalWrite(LED_BUILTIN, HIGH);
            Serial1.println("Received packet");
            // RH_RF95::printBuffer("Received: ", buf, len);
            // Serial1.print("Got: ");
            // Serial1.println((char*)buf);
            // Serial1.print("Packet millis(): ");
            // Serial1.println(packet->raw_adc_value);
            Serial1.print("RSSI: ");
            Serial1.println(rf95.lastRssi(), DEC);
            digitalWrite(LED_BUILTIN, LOW);
            return 1;
        }
        else
        {
            Serial1.println("Receive failed");
        }
    }



    return 0;

}