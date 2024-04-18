#include "cryo_system.h"
#include "cryo_adc.h"

ADCDifferential::ADCDifferential(
  ADCDifferential::INPUT_PIN_POS input_pos,
  ADCDifferential::INPUT_PIN_NEG input_neg,
  ADCDifferential::GAIN gain,
  ADCDifferential::AVERAGES averages,
  ADCDifferential::RESOLUTION resolution,
  ADCDifferential::VOLTAGE_REFERENCE reference
) {

  this->input_pos = input_pos;
  this->input_neg = input_neg;
  this->gain = gain;
  this->averages = averages;
  this->resolution = resolution;
  this->reference = reference;

}

ADCDifferential::~ADCDifferential() {
  this->disable();
}

void ADCDifferential::begin() {
  
  // Generic clock init
  this->generic_clock_init();
  CRYO_DEBUG_MESSAGE("Clock init");
  // ADC init
  this->disable();
  CRYO_DEBUG_MESSAGE("ADC disabled");
  this->adc_init();
  CRYO_DEBUG_MESSAGE("ADC initialied");
  
  CRYO_DEBUG_MESSAGE("Setting input pins");
  this->set_input_pins(this->input_pos, this->input_neg);  
  CRYO_DEBUG_MESSAGE("Input pins done");
  
  this->set_gain(this->gain);  
  CRYO_DEBUG_MESSAGE("Gain done");
  
  this->set_resolution(this->resolution);
  CRYO_DEBUG_MESSAGE("Resolution done");
  
  this->set_voltage_reference(this->reference);
  CRYO_DEBUG_MESSAGE("Voltage reference done");
  
  
  this->set_averages(this->averages);
  CRYO_DEBUG_MESSAGE("ADC configured");
  
}

void ADCDifferential::generic_clock_init() {
    
  // Step 1 - Configure Generic Clock Generator (GCLK_ADC)
  // Unless we want to reduce the conversion rate, we can leave GENDIV alone
    // GCLK->GENDIV = ?;
    // assign source (GENDIV.ID)
    // assign division factor (GENDIV.DIV)
  GCLK->GENDIV.reg =
    GCLK_GENDIV_ID(1) |
    GCLK_GENDIV_DIV(1);
  while (GCLK->STATUS.bit.SYNCBUSY);
  
  // // Configure Generic Clock Control
  // // assign GENCTRL.IcD
  // // assign source of generic clock generator (CLKCTRL.GEN)
  // // Bits 15   - Write Lock = 0
  // // Bits 14   - Clock Enable = 1
  // // - 15:12 = 0x4
  // // Bits 11:8 - Clock Generator = 0x0 (GCLKGEN0)
  // // - 11:18 = 0x0
  // // Bits 5:0  - Generic Clock Select ID = 0x1E (GCLK_ADC)
  // // -----------------------------------------------------
  GCLK->CLKCTRL.reg = 0x401e;
  while (GCLK->STATUS.bit.SYNCBUSY);

}

void ADCDifferential::adc_init() {

  ADC->CTRLA.reg = ADC_CTRLA_SWRST;
  this->wait_for_sync();
  
  // // Step 3 - Configure Differential Mode
  // //   this will resolve the voltage between MUXPOS and MUXNEG
  ADC->CTRLB.reg = 
    // set 12-bit resolution
    // divide clock by 512 (8 MHz / 512 = 15.62 kHz)
    ADC_CTRLB_PRESCALER_DIV512 |
    // and enable differential mode
    ADC_CTRLB_DIFFMODE;
  this->wait_for_sync();

  // Set sampling time 
  //    JH: need to play around here with effect on ADC input impedance/accuracy
    ADC->SAMPCTRL.reg = 
    ADC_SAMPCTRL_MASK & 0; // 0xff;
  this->wait_for_sync();

  // Step 6 - Read NVM calibration values
  // - ref: https://blog.thea.codes/reading-analog-values-with-the-samd-adc/
  uint32_t bias = (*((uint32_t *) ADC_FUSES_BIASCAL_ADDR) & ADC_FUSES_BIASCAL_Msk) >> ADC_FUSES_BIASCAL_Pos;
  uint32_t linearity = (*((uint32_t *) ADC_FUSES_LINEARITY_0_ADDR) & ADC_FUSES_LINEARITY_0_Msk) >> ADC_FUSES_LINEARITY_0_Pos;
  linearity |= ((*((uint32_t *) ADC_FUSES_LINEARITY_1_ADDR) & ADC_FUSES_LINEARITY_1_Msk) >> ADC_FUSES_LINEARITY_1_Pos) << 5;
 
  /* Write the calibration data. */
  ADC->CALIB.reg = ADC_CALIB_BIAS_CAL(bias) | ADC_CALIB_LINEARITY_CAL(linearity);
  /* Wait for bus synchronization. */
  while (ADC->STATUS.bit.SYNCBUSY) {};

}

void ADCDifferential::set_input_pins(ADCDifferential::INPUT_PIN_POS input_pos, ADCDifferential::INPUT_PIN_NEG input_neg) {


  CRYO_DEBUG_MESSAGE("Disabling ADC");
  bool enabled = this->is_enabled();
  if (enabled)
    this->disable();


  CRYO_DEBUG_MESSAGE("Setting pin direction");
    // Assign port direction registers
  this->input_pin_direction(input_pos);
  this->input_pin_direction(input_neg);
  
  CRYO_DEBUG_MESSAGE("Assigning register");
    ADC->INPUTCTRL.reg = (ADC->INPUTCTRL.reg & ~(ADC_INPUTCTRL_MUXPOS_Msk | ADC_INPUTCTRL_MUXNEG_Msk)) 
    | (uint32_t)input_pos | (uint32_t)input_neg;
  this->wait_for_sync();

  CRYO_DEBUG_MESSAGE("Waiting for sync");
  
  if (enabled)
    this->enable();

}

void ADCDifferential::input_pin_direction(ADCDifferential::INPUT_PIN_POS input_pos) {

  // JH: moved from switch statement to if/else - stopped bug where multiple pin directions were being set

  if (input_pos == INPUT_PIN_POS::A0_PIN)
      this->input_pin_direction_register_set(GROUP_0, PORT_PA02, 2, 1, PORT_PMUX_PMUXE_A);
  else if (input_pos == INPUT_PIN_POS::A1_PIN)
      this->input_pin_direction_register_set(GROUP_1, PORT_PB08, 8, 4, PORT_PMUX_PMUXE_B);
  else if (input_pos == INPUT_PIN_POS::A2_PIN)
      this->input_pin_direction_register_set(GROUP_1, PORT_PB08, 9, 4, PORT_PMUX_PMUXO_B);
  else if (input_pos == INPUT_PIN_POS::A3_PIN)
      this->input_pin_direction_register_set(GROUP_0, PORT_PA04, 4, 2, PORT_PMUX_PMUXE_A);
  else if (input_pos == INPUT_PIN_POS::A4_PIN)
      this->input_pin_direction_register_set(GROUP_0, PORT_PA05, 5, 2, PORT_PMUX_PMUXO_A);
  else if (input_pos == INPUT_PIN_POS::A5_PIN)
      this->input_pin_direction_register_set(GROUP_1, PORT_PB02, 2, 1, PORT_PMUX_PMUXE_B);
  else if (input_pos == INPUT_PIN_POS::D0_PIN)
      this->input_pin_direction_register_set(GROUP_0, PORT_PA11, 11, 5, PORT_PMUX_PMUXO_A);
  else if (input_pos == INPUT_PIN_POS::D1_PIN)
      this->input_pin_direction_register_set(GROUP_0, PORT_PA10, 10, 5, PORT_PMUX_PMUXE_A);
  else if (input_pos == INPUT_PIN_POS::D9_PIN)
      this->input_pin_direction_register_set(GROUP_0, PORT_PA07, 7, 3, PORT_PMUX_PMUXO_A);
  
  return; // don't need to do anything

}

void ADCDifferential::input_pin_direction(ADCDifferential::INPUT_PIN_NEG input_neg) {
  
  // JH: moved from switch statement to if/else - stopped bug where multiple pin directions were being set

  if (input_neg == INPUT_PIN_NEG::A0_PIN)
      this->input_pin_direction_register_set(GROUP_0, PORT_PA02, 2, 1, PORT_PMUX_PMUXE_A);
  else if (input_neg == INPUT_PIN_NEG::A1_PIN)
      this->input_pin_direction_register_set(GROUP_1, PORT_PB08, 8, 4, PORT_PMUX_PMUXE_B);
  else if (input_neg == INPUT_PIN_NEG::A2_PIN)
      this->input_pin_direction_register_set(GROUP_1, PORT_PB08, 9, 4, PORT_PMUX_PMUXO_B);
  else if (input_neg == INPUT_PIN_NEG::A3_PIN)
      this->input_pin_direction_register_set(GROUP_0, PORT_PA04, 4, 2, PORT_PMUX_PMUXE_A);
  else if (input_neg == INPUT_PIN_NEG::A4_PIN)
      this->input_pin_direction_register_set(GROUP_0, PORT_PA05, 5, 2, PORT_PMUX_PMUXO_A);
  else if (input_neg == INPUT_PIN_NEG::D9_PIN)
      this->input_pin_direction_register_set(GROUP_0, PORT_PA07, 7, 3, PORT_PMUX_PMUXO_A);

  return; // don't need to do anything

}

void ADCDifferential::input_pin_direction_register_set(
  uint8_t group,
  uint32_t dirclr,
  uint16_t pincfg,
  uint16_t pmux,
  uint8_t pmuxreg) {

  CRYO_DEBUG_MESSAGE("Setting pin direction (register level)");
  PORT->Group[group].DIRCLR.reg = dirclr; // DIRCLR for input, DIRSET for output
  PORT->Group[group].PINCFG[pincfg].reg |= PORT_PINCFG_PMUXEN;
  PORT->Group[group].PMUX[pmux].reg = pmuxreg;
  this->wait_for_sync();
  CRYO_DEBUG_MESSAGE("Setting pin direction (finished waiting)");

}

void ADCDifferential::enable() {

  ADC->CTRLA.reg = ADC->CTRLA.reg | ADC_CTRLA_ENABLE;
  this->wait_for_sync();

}

void ADCDifferential::disable() {

  ADC->CTRLA.reg = ADC->CTRLA.reg & ~ADC_CTRLA_ENABLE;
  this->wait_for_sync();

}

bool ADCDifferential::is_enabled() {

  // return enable bit
  return (bool) ADC->CTRLA.bit.ENABLE;

}

void ADCDifferential::wait_for_sync() {
  
  while (ADC->STATUS.bit.SYNCBUSY);

}

void ADCDifferential::set_gain(ADCDifferential::GAIN gain) {

  // Store enabled state
  bool enabled = this->is_enabled();
  if (enabled)
    this->disable();

  // Clear existing gain setting, then OR with new one
  ADC->INPUTCTRL.reg = (ADC->INPUTCTRL.reg & ~ADC_INPUTCTRL_GAIN_Msk) | gain;
  this->wait_for_sync();

  // Re-enable if necessary
  if (enabled) this->enable();

}

void ADCDifferential::set_gain(float_t gain_numeric) {

  // Perform conversion and defer to register implementation
  ADCDifferential::GAIN gain_enum = ADCDifferential::convert_gain_to_enum(gain_numeric);
  this->set_gain(gain_enum);

}

void ADCDifferential::set_voltage_reference(ADCDifferential::VOLTAGE_REFERENCE reference) {

  // Store enabled state
  bool enabled = this->is_enabled();
  if (enabled)
    this->disable();
    
  // Clear existing voltage reference, then OR with new one
  ADC->REFCTRL.reg =(ADC->REFCTRL.reg & ~ADC_REFCTRL_REFSEL_Msk) | ADC_REFCTRL_REFCOMP | reference;
  this->wait_for_sync();

  // Re-enable if necessary
  if (enabled) this->enable();

}

void ADCDifferential::set_resolution(ADCDifferential::RESOLUTION res) {

  // Store enabled state
  bool enabled = this->is_enabled();
  if (enabled)
    this->disable();
    
  // Clear existing voltage reference, then OR with new one
  ADC->CTRLB.reg =(ADC->CTRLB.reg & ~ADC_CTRLB_RESSEL_Msk) | res;
  this->wait_for_sync();

  // Re-enable if necessary
  if (enabled) this->enable();

}

void ADCDifferential::set_averages(ADCDifferential::AVERAGES averages) {

  bool enabled = this->is_enabled();
  if (enabled)
    this->disable();

    ADC->AVGCTRL.reg = (ADC->AVGCTRL.reg & ~ADC_AVGCTRL_SAMPLENUM_Msk) | averages;
    this->wait_for_sync();

  if (enabled) this->enable();

}

int16_t ADCDifferential::read() {
  
  // Read ADC value
  int16_t adc_conversion;
  // - Trigger conversion
  ADC->SWTRIG.reg = ADC_SWTRIG_START;
  this->wait_for_sync();

  // int16_t adc_conversion;
  // // wait until ready
  while (ADC->INTFLAG.bit.RESRDY == 0) {} 
  // store value
  adc_conversion = ADC->RESULT.reg;

  return adc_conversion;

}

ADCDifferential::VOLTAGE_REFERENCE ADCDifferential::get_voltage_reference() {
  return this->reference;
}

ADCDifferential::GAIN ADCDifferential::get_gain() {
  return this->gain;
}

float_t ADCDifferential::get_gain_numeric() {
  if (this->gain == ADCDifferential::GAIN::GAIN_1X) {
    return 1.0;
  } else if (this->gain == ADCDifferential::GAIN::GAIN_2X) {
    return 2.0;
  } else if (this->gain == ADCDifferential::GAIN::GAIN_4X) {
    return 4.0;
  } else if (this->gain == ADCDifferential::GAIN::GAIN_8X) {
    return 8.0;
  } else if (this->gain == ADCDifferential::GAIN::GAIN_16X) {
    return 16.0;
  } else if (this->gain == ADCDifferential::GAIN::GAIN_DIV2) {
    return 0.5;
  } 
  return 0;
}