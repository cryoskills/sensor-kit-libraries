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
    cryo_adc.h
DESCRIPTION: 
    Provides an interface to configure and read from the SAMD21 
    analogue-to-digital converter operating in differential mode, 
    as integated with the Adafruit Feather M0 development board. 
    Pin identifiers correspond to the 'Arduino Name' for each pin.

CONFIGURATION:
    
    input_pos
        description:    positive input pin assigned to the ADC amplifier
        type:           ADCDifferential::INPUT_PIN_POS
        values:         see list of possible pins defined by ADCDifferential::INPUT_PIN_POS

    input_neg
        description:    negative input pin assigned to the ADC amplifier
        type:           ADCDifferential::INPUT_PIN_NEG
        values:         see list of possible pins defined by ADCDifferential::INPUT_PIN_NEG

    gain
        description:    sets the gain of the ADC amplifier (prior to digitisation).
                        For example, 50mV differential voltage from (input_pos) to (input_neg)
                        and a gain of 4x will result in a voltage of 200mV being digitised.
        type:           ADCDifferential::GAIN
        values:         0.5x, 1x, 2x, 4x, 8x, 16x
    
    averages
        description:    sets the number of averages that the ADC should use
                        to calculate the final value returned by read().
                        NOTE: requires that the resolution is set to 
                        ADCDifferential::RESOLUTION::RES_AVG in order to
                        work correctly, which is equivalent to 16 bits.
        type:           ADCDifferential::AVERAGES
        values:         NO_AVERAGING, 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024

    resolution:
        description:    sets the resolution of the value returned by read().
                        NOTE: must be set to RES_AVG if averaging is being used.
                        Care should be taken when averaging to check whether
                        adjustment of the value has taken place automatically.
                        See 33.6.6 in SAMD21 datasheet for details.
        type:           ADCDifferential::RESOLUTION
        values:         8_BIT, 10_BIT, 12_BIT

    voltage_reference:
        description:    sets the reference used in converting the voltage to a
                        digital value.
        type:           ADCDifferential::VOLTAGE_REFERENCE
        values:         INT1V_INTERNAL (1.0 volt reference)
                        INTVCC0_INTERNAL (1/1.48 * VDD reference), 
                        INTVCC1_INTERNAL (1/2 * VDD reference), 
                        AREF_PIN (VREFB), 
                        A3_PIN (A3)


EXAMPLE USAGE:

    #include "Arduino.h" 
    #include "cryo_adc.h"

    ADCDifferential my_adc(
        ADCDifferential::INPUT_PIN_POS::A1_PIN,
        ADCDifferential::INPUT_PIN_NEG::GND
    );

    void setup() {
        Serial.begin(9600);
        my_acc.begin();
        my_adc.enable();
    }

    void loop() {
        int16_t my_adc_result = my_adc.read();
        Serial.print("Result: ");
        Serial.println(my_adc_result);
    }

******************************************************************************/

#include <Arduino.h>

#ifndef CRYO_ADC_H
#define CRYO_ADC_H

// Simple macros to make sure we're using the right numbers for GPIO group IDs
#define GROUP_0 0
#define GROUP_1 1

/*

    class ADCDifferential
    description:
        interface to SAMD21 ADC in differential mode, as implemented on the 
        Adafruit Adalogger Feather M0.

*/
class ADCDifferential {

    public:
    // Input pin definitions for positive ADC input (33.8.8)
    enum class INPUT_PIN_POS {
        A0_PIN = ADC_INPUTCTRL_MUXPOS_PIN0,
        A1_PIN = ADC_INPUTCTRL_MUXPOS_PIN2,
        A2_PIN = ADC_INPUTCTRL_MUXPOS_PIN3,
        A3_PIN = ADC_INPUTCTRL_MUXPOS_PIN4,
        A4_PIN = ADC_INPUTCTRL_MUXPOS_PIN5,
        A5_PIN = ADC_INPUTCTRL_MUXPOS_PIN10,
        D0_PIN = ADC_INPUTCTRL_MUXPOS_PIN9,
        D1_PIN = ADC_INPUTCTRL_MUXPOS_PIN8,
        D9_PIN = ADC_INPUTCTRL_MUXPOS_PIN7,
        DAC_PIN = ADC_INPUTCTRL_MUXPOS_DAC,
        VREF_BANDGAP_INTERNAL = ADC_INPUTCTRL_MUXPOS_BANDGAP,
        VREF_TEMP_INTERNAL = ADC_INPUTCTRL_MUXPOS_TEMP,
        VREF_SCALEDCOREVCC_INTERNAL = ADC_INPUTCTRL_MUXPOS_SCALEDCOREVCC,
        VREF_SCALEDIOVCC_INTERNAL = ADC_INPUTCTRL_MUXPOS_SCALEDIOVCC
    };

    // Input pin defintions for negative ADC input (33.8.8)
    enum class INPUT_PIN_NEG {
        A0_PIN = ADC_INPUTCTRL_MUXNEG_PIN0,
        A1_PIN = ADC_INPUTCTRL_MUXNEG_PIN2,
        A2_PIN = ADC_INPUTCTRL_MUXNEG_PIN3,
        A3_PIN = ADC_INPUTCTRL_MUXNEG_PIN4,
        A4_PIN = ADC_INPUTCTRL_MUXNEG_PIN5,
        D9_PIN = ADC_INPUTCTRL_MUXNEG_PIN7,
        GND = ADC_INPUTCTRL_MUXNEG_GND
    };

    // Voltage reference options as specified in 33.8.2
    enum VOLTAGE_REFERENCE {
        INT1V_INTERNAL = ADC_REFCTRL_REFSEL_INT1V,
        INTVCC0_INTERNAL = ADC_REFCTRL_REFSEL_INTVCC0,
        INTVCC1_INTERNAL = ADC_REFCTRL_REFSEL_INTVCC1,
        AREF_PIN = ADC_REFCTRL_REFSEL_AREFA,
        A3_PIN = ADC_REFCTRL_REFSEL_AREFA
    };

    // ADC result resolution as specified in 33.8.5
    enum RESOLUTION {
        RES_12BIT = ADC_CTRLB_RESSEL_12BIT,
        RES_10BIT = ADC_CTRLB_RESSEL_10BIT,
        RES_AVG = ADC_CTRLB_RESSEL_16BIT,
        RES_8BIT = ADC_CTRLB_RESSEL_8BIT
    };

    // ADC pre-conversion gain, as specified in 33.8.8
    enum GAIN {
        GAIN_1X = ADC_INPUTCTRL_GAIN_1X,
        GAIN_2X = ADC_INPUTCTRL_GAIN_2X,
        GAIN_4X = ADC_INPUTCTRL_GAIN_4X,
        GAIN_8X = ADC_INPUTCTRL_GAIN_8X,
        GAIN_16X = ADC_INPUTCTRL_GAIN_16X,
        GAIN_DIV2 = ADC_INPUTCTRL_GAIN_DIV2,
    };


    // Number of averages used for final result, as specified in 33.8.3
    enum AVERAGES {
        NO_AVERAGING = ADC_AVGCTRL_SAMPLENUM_1,
        AVG_X2 = ADC_AVGCTRL_SAMPLENUM_2,
        AVG_X4 = ADC_AVGCTRL_SAMPLENUM_4,
        AVG_X8 = ADC_AVGCTRL_SAMPLENUM_8,
        AVG_X16 = ADC_AVGCTRL_SAMPLENUM_16,
        AVG_X32 = ADC_AVGCTRL_SAMPLENUM_32,
        AVG_X64 = ADC_AVGCTRL_SAMPLENUM_64,
        AVG_X128 = ADC_AVGCTRL_SAMPLENUM_128,
        AVG_X256 = ADC_AVGCTRL_SAMPLENUM_256,
        AVG_X512 = ADC_AVGCTRL_SAMPLENUM_512,
        AVG_X1024 = ADC_AVGCTRL_SAMPLENUM_1024,
    };

    private:
        // Input and output pins
        ADCDifferential::INPUT_PIN_POS input_pos;
        ADCDifferential::INPUT_PIN_NEG input_neg;

        // Resolution
        ADCDifferential::RESOLUTION resolution;

        // Voltage reference
        ADCDifferential::VOLTAGE_REFERENCE reference;

        // Gain
        ADCDifferential::GAIN gain;

        // Averages
        ADCDifferential::AVERAGES averages;

    public:
        // Convert a gain value to the nearest accepted value and return the corresponding enum
        static ADCDifferential::GAIN convert_gain_to_enum(float_t gain);
        // Convert a gain enum to the corresponding numeric gain
        static float_t convert_enum_to_gain(ADCDifferential::GAIN);

        // Class constructor
        ADCDifferential(
            ADCDifferential::INPUT_PIN_POS input_pos,
            ADCDifferential::INPUT_PIN_NEG input_neg,
            ADCDifferential::GAIN gain = ADCDifferential::GAIN::GAIN_1X,
            ADCDifferential::AVERAGES averages = ADCDifferential::AVERAGES::AVG_X1024,
            ADCDifferential::RESOLUTION resolution = ADCDifferential::RESOLUTION::RES_AVG,
            ADCDifferential::VOLTAGE_REFERENCE reference = ADCDifferential::VOLTAGE_REFERENCE::INT1V_INTERNAL
        );

        // Class destructor
        //  called when the class is out of scope - used to disable the ADC
        ~ADCDifferential();

        // Initialise the ADC with the parameters specified (calls clock and ADC init functions)
        void begin();

        // enable the ADC
        void enable();
        // disable the ADC
        void disable();

        // check whether the ADC is enabled
        bool is_enabled();

        // Read ADC value as a right-adjusted, 16-bit signed integer
        int16_t read();

        /********************************************************************/
        /* SETTERS                                                          */
        /********************************************************************/
        void set_input_pins(
            ADCDifferential::INPUT_PIN_POS input_pos,
            ADCDifferential::INPUT_PIN_NEG input_neg
        );

        void set_resolution(
            ADCDifferential::RESOLUTION resolution
        );

        void set_voltage_reference(
            ADCDifferential::VOLTAGE_REFERENCE reference
        );

        void set_gain(
            ADCDifferential::GAIN gain
        );
        // overload to covert from numeric gain
        void set_gain(
            float_t gain
        );

        void set_averages(ADCDifferential::AVERAGES averages);

        /********************************************************************/
        /* GETTERS                                                          */
        /********************************************************************/
        ADCDifferential::INPUT_PIN_POS get_input_pin_positive();
        ADCDifferential::INPUT_PIN_POS get_input_pin_negative();

        ADCDifferential::RESOLUTION get_resolution();
        uint8_t get_resolution_numeric();

        ADCDifferential::VOLTAGE_REFERENCE get_voltage_reference();
        ADCDifferential::GAIN get_gain();
        // returns the gain of the ADC in numeric form
        float_t get_gain_numeric();

    private:
        void generic_clock_init();
        void adc_init();
        void wait_for_sync();

        void input_pin_direction(ADCDifferential::INPUT_PIN_POS input_pos);
        void input_pin_direction(ADCDifferential::INPUT_PIN_NEG input_neg);

        void input_pin_direction_register_set(
            uint8_t group,
            uint32_t dirclr,
            uint16_t pincfg,
            uint16_t pmux,
            uint8_t pmuxreg
        );

};

#endif