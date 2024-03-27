#include "cryo_radio.h"
#include "RH_RF95.h"

RH_RF95 rf95(
    CRYO_RADIO_CS_PIN,
    CRYO_RADIO_IRQ_PIN
);

int32_t cryo_radio_init() {
    
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

    return 1;

}

void cryo_radio_enable() {

    digitalWrite(CRYO_RADIO_ENABLE_PIN, HIGH);

}

void cryo_radio_disable() {

    digitalWrite(CRYO_RADIO_ENABLE_PIN, LOW);

}

int16_t packetnum = 0; 

int32_t cryo_radio_send_packet(cryo_radio_packet* packet) {

    // create temporary char buffer to store packet
    uint8_t data[sizeof(*packet)];
    // copy packet to char array
    Serial1.println("Copying packet");
    memcpy(data, packet, sizeof(*packet));

    // // send data
    // Serial1.println("Enabling radio");
    cryo_radio_enable();

    Serial1.print("cryo_radio_packet is bytes long: ");
    Serial1.println(sizeof(*packet));

    Serial1.println("Sending to rf95_server");
    // Send a message to rf95_server
    // char radiopacket[20] = "Hello World # ";
    // itoa(packetnum++, radiopacket+13, 10);
    // Serial1.print("Sending"); Serial1.println(radiopacket);
    // radiopacket[19] = 0; 
    Serial1.println("Sending..."); delay(10) ;
    rf95.send((uint8_t *) packet, sizeof(*packet));
    Serial1.println("Waiting for packet to complete..."); delay(10);
    rf95.waitPacketSent();


    // Serial1.println("sending dummy packet");

    // char data2[30] = "And hello back to you";
    // data2[29] = 0;
    // if (!rf95.send((uint8_t*)data2, sizeof(data2)))
    //     Serial1.println("Failed to send!");

    // Serial1.println("waiting dummy sent");
    // Serial1.flush();
    // delay(100);
    // rf95.waitPacketSent();

    // Serial.println("Finished sending dummy packet");

    // Serial1.println("Sending data");
    // if (!rf95.send(data, sizeof(data)))
    //     Serial1.println("error sending");
    // Serial1.println("Sent data");
    // // delay(100);
    // // // rf95.setModeIdle();
    // Serial1.println("Finished waiting");
    // Serial1.flush();
    // delay(100);
    // rf95.waitPacketSent();

    // Serial1.println("Finished waiting for packet sent");
    // delay(10);

    // // wait until done to disable
    cryo_radio_disable();
    Serial1.println("Disabling radio");

    
    // // Do something with the packet
    // return sizeof(packet);

    return 1;

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