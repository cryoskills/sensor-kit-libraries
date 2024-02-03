#include "cryo_adc.h"

ADCDifferential::ADCDifferential(
  ADCDifferential::INPUT_PIN_POS input_pos,
  ADCDifferential::INPUT_PIN_NEG input_neg,
  ADCDifferential::GAIN gain,
  ADCDifferential::AVERAGES averages,
  ADCDifferential::RESOLUTION resolution,
  ADCDifferential::VOLTAGE_REFERENCE reference
) {

  // Generic clock init
  this->generic_clock_init();
  // ADC init
  this->disable();
  this->adc_init();

  this->set_input_pins(input_pos, input_neg);
  this->set_gain(gain);
  this->set_resolution(resolution);
  this->set_voltage_reference(reference);

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

  bool enabled = this->is_enabled();
  if (enabled)
    this->disable();

  // Assign port direction registers
  this->input_pin_direction(input_pos);
  this->input_pin_direction(input_neg);
  
  ADC->INPUTCTRL.reg = (ADC->INPUTCTRL.reg & ~(ADC_INPUTCTRL_MUXPOS_Msk | ADC_INPUTCTRL_MUXNEG_Msk)) 
    | (uint32_t)input_pos | (uint32_t)input_neg;
  this->wait_for_sync();

  if (enabled)
    this->enable();

}

void ADCDifferential::input_pin_direction(ADCDifferential::INPUT_PIN_POS input_pos) {

  switch (input_pos) {
    case INPUT_PIN_POS::A0_PIN:
      this->input_pin_direction_register_set(GROUP_0, PORT_PA02, 2, 1, PORT_PMUX_PMUXE_A);
    case INPUT_PIN_POS::A1_PIN:
      this->input_pin_direction_register_set(GROUP_1, PORT_PB08, 8, 4, PORT_PMUX_PMUXE_B);
    case INPUT_PIN_POS::A2_PIN:
      this->input_pin_direction_register_set(GROUP_1, PORT_PB08, 9, 4, PORT_PMUX_PMUXO_B);
    case INPUT_PIN_POS::A3_PIN:
      this->input_pin_direction_register_set(GROUP_0, PORT_PA04, 4, 2, PORT_PMUX_PMUXE_A);
    case INPUT_PIN_POS::A4_PIN:
      this->input_pin_direction_register_set(GROUP_0, PORT_PA05, 5, 2, PORT_PMUX_PMUXO_A);
    case INPUT_PIN_POS::A5_PIN:
      this->input_pin_direction_register_set(GROUP_1, PORT_PB02, 2, 1, PORT_PMUX_PMUXE_B);
    case INPUT_PIN_POS::D0_PIN:
      this->input_pin_direction_register_set(GROUP_0, PORT_PA11, 11, 5, PORT_PMUX_PMUXO_A);
    case INPUT_PIN_POS::D1_PIN:
      this->input_pin_direction_register_set(GROUP_0, PORT_PA10, 10, 5, PORT_PMUX_PMUXE_A);
    case INPUT_PIN_POS::D9_PIN:
      this->input_pin_direction_register_set(GROUP_0, PORT_PA07, 7, 3, PORT_PMUX_PMUXO_A);
    default:
      return; // don't need to do anything
  }

}

void ADCDifferential::input_pin_direction(ADCDifferential::INPUT_PIN_NEG input_neg) {

  switch (input_pos) {
    case INPUT_PIN_POS::A0_PIN:
      this->input_pin_direction_register_set(GROUP_0, PORT_PA02, 2, 1, PORT_PMUX_PMUXE_A);
    case INPUT_PIN_POS::A1_PIN:
      this->input_pin_direction_register_set(GROUP_1, PORT_PB08, 8, 4, PORT_PMUX_PMUXE_B);
    case INPUT_PIN_POS::A2_PIN:
      this->input_pin_direction_register_set(GROUP_1, PORT_PB08, 9, 4, PORT_PMUX_PMUXO_B);
    case INPUT_PIN_POS::A3_PIN:
      this->input_pin_direction_register_set(GROUP_0, PORT_PA04, 4, 2, PORT_PMUX_PMUXE_A);
    case INPUT_PIN_POS::A4_PIN:
      this->input_pin_direction_register_set(GROUP_0, PORT_PA05, 5, 2, PORT_PMUX_PMUXO_A);
    case INPUT_PIN_POS::D9_PIN:
      this->input_pin_direction_register_set(GROUP_0, PORT_PA07, 7, 3, PORT_PMUX_PMUXO_A);
    default:
      return; // don't need to do anything
  }

}

void ADCDifferential::input_pin_direction_register_set(
  uint8_t group,
  uint32_t dirclr,
  uint16_t pincfg,
  uint16_t pmux,
  uint8_t pmuxreg) {

  PORT->Group[group].DIRCLR.reg = dirclr; // DIRCLR for input, DIRSET for output
  PORT->Group[group].PINCFG[pincfg].reg |= PORT_PINCFG_PMUXEN;
  PORT->Group[group].PMUX[pmux].reg = pmuxreg;

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
  
  this->wait_for_sync();

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