// TODO: Add documentation here
void cryo_init_adc();

void cryo_configure_adc(
    uint16_t pin_pos, 
    uint16_t pin_neg, 
    uint16_t voltage_reference,
    uint16_t    
);

int32_t cryo_read_adc();