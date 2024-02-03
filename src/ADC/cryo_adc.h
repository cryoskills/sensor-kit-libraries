// TODO: Add documentation here
#include <Arduino.h>

#define GROUP_0 0
#define GROUP_1 1

class ADCDifferential {

    public:
    // Define input pin definitions for positive ADC input
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

    enum class INPUT_PIN_NEG {
        A0_PIN = ADC_INPUTCTRL_MUXNEG_PIN0,
        A1_PIN = ADC_INPUTCTRL_MUXNEG_PIN2,
        A2_PIN = ADC_INPUTCTRL_MUXNEG_PIN3,
        A3_PIN = ADC_INPUTCTRL_MUXNEG_PIN4,
        A4_PIN = ADC_INPUTCTRL_MUXNEG_PIN5,
        D9_PIN = ADC_INPUTCTRL_MUXNEG_PIN7,
        GND = ADC_INPUTCTRL_MUXNEG_GND
    };

    enum VOLTAGE_REFERENCE {
        INT1V_INTERNAL = ADC_REFCTRL_REFSEL_INT1V,
        INTVCC0_INTERNAL = ADC_REFCTRL_REFSEL_INTVCC0,
        INTVCC1_INTERNAL = ADC_REFCTRL_REFSEL_INTVCC1,
        AREF_PIN = ADC_REFCTRL_REFSEL_AREFA,
        A3_PIN = ADC_REFCTRL_REFSEL_AREFA
    };

    enum RESOLUTION {
        RES_12BIT = ADC_CTRLB_RESSEL_12BIT,
        RES_10BIT = ADC_CTRLB_RESSEL_10BIT,
        RES_AVG = ADC_CTRLB_RESSEL_16BIT,
        RES_8BIT = ADC_CTRLB_RESSEL_8BIT
    };

    enum GAIN {
        GAIN_1X = ADC_INPUTCTRL_GAIN_1X,
        GAIN_2X = ADC_INPUTCTRL_GAIN_2X,
        GAIN_4X = ADC_INPUTCTRL_GAIN_4X,
        GAIN_8X = ADC_INPUTCTRL_GAIN_8X,
        GAIN_16X = ADC_INPUTCTRL_GAIN_16X,
        GAIN_DIV2 = ADC_INPUTCTRL_GAIN_DIV2,
    };

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
        uint16_t samples;
        float_t gain;

        // Input and output pins
        ADCDifferential::INPUT_PIN_POS input_pos;
        ADCDifferential::INPUT_PIN_NEG input_neg;

        // Resolution
        ADCDifferential::RESOLUTION resolution;

        // Voltage reference
        ADCDifferential::VOLTAGE_REFERENCE reference;

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
            ADCDifferential::AVERAGES averages = ADCDifferential::AVERAGES::NO_AVERAGING,
            ADCDifferential::RESOLUTION resolution = ADCDifferential::RESOLUTION::RES_AVG,
            ADCDifferential::VOLTAGE_REFERENCE reference = ADCDifferential::VOLTAGE_REFERENCE::INT1V_INTERNAL
        );

        // Class destructor
        //  called when the class is out of scope - used to disable the ADC
        ~ADCDifferential();

        // Read ADC value
        int16_t read();

        // Getters and setters
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

        ADCDifferential::INPUT_PIN_POS get_input_pin_positive();
        ADCDifferential::INPUT_PIN_POS get_input_pin_negative();

        ADCDifferential::RESOLUTION get_resolution();
        uint8_t get_resolution_numeric();

        ADCDifferential::VOLTAGE_REFERENCE get_voltage_reference();
        ADCDifferential::GAIN get_gain();
        float_t get_gain_numeric();

        bool is_enabled();

        void enable();
        void disable();

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

void cryo_init_adc();

void cryo_configure_adc(
    uint16_t pin_pos, 
    uint16_t pin_neg, 
    uint16_t voltage_reference
);

int32_t cryo_read_adc();