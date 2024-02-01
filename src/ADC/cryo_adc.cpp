/* BEGIN function _defintions_ */
void cryo_init_adc(void) {
    
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

  ADC->CTRLA.reg = ADC_CTRLA_SWRST;
  while (ADC->STATUS.bit.SYNCBUSY);

  // // Step 3 - Configure Differential Mode
  // //   this will resolve the voltage between MUXPOS and MUXNEG
  ADC->CTRLB.reg = 
    // set 12-bit resolution
    ADC_CTRLB_RESSEL_16BIT |
    // divide clock by 512 (8 MHz / 512 = 15.62 kHz)
    ADC_CTRLB_PRESCALER_DIV512 |
    // and enable differential mode
    ADC_CTRLB_DIFFMODE;
  while (ADC->STATUS.bit.SYNCBUSY);

  // Set sampling time 
  //    JH: need to play around here with effect on ADC input impedance/accuracy
    ADC->SAMPCTRL.reg = 
    ADC_SAMPCTRL_MASK & 0; // 0xff;
  while (ADC->STATUS.bit.SYNCBUSY);

  // Step 6 - Read NVM calibration values
  // - ref: https://blog.thea.codes/reading-analog-values-with-the-samd-adc/
  uint32_t bias = (*((uint32_t *) ADC_FUSES_BIASCAL_ADDR) & ADC_FUSES_BIASCAL_Msk) >> ADC_FUSES_BIASCAL_Pos;
  uint32_t linearity = (*((uint32_t *) ADC_FUSES_LINEARITY_0_ADDR) & ADC_FUSES_LINEARITY_0_Msk) >> ADC_FUSES_LINEARITY_0_Pos;
  linearity |= ((*((uint32_t *) ADC_FUSES_LINEARITY_1_ADDR) & ADC_FUSES_LINEARITY_1_Msk) >> ADC_FUSES_LINEARITY_1_Pos) << 5;
 
  /* Write the calibration data. */
  ADC->CALIB.reg = ADC_CALIB_BIAS_CAL(bias) | ADC_CALIB_LINEARITY_CAL(linearity);
  /* Wait for bus synchronization. */
  while (ADC->STATUS.bit.SYNCBUSY) {};

  // Step 7 - Configure input ports
  // PIN A2 on Adafruit Feather M0
  //  clear direction register for PB09 (sets to input)
  PORT->Group[1].DIRCLR.reg = PORT_PB09; // Pin A2
  PORT->Group[1].PINCFG[9].reg |= PORT_PINCFG_PMUXEN;
  PORT->Group[1].PMUX[4].reg = PORT_PMUX_PMUXO_B; // note PMUXO for ODD pin
  while (ADC->STATUS.bit.SYNCBUSY);

  // PIN A1 on Adafruit Feather M0
  PORT->Group[1].DIRCLR.reg = PORT_PB08; // Pin A1
  PORT->Group[1].PINCFG[8].reg |= PORT_PINCFG_PMUXEN;
  PORT->Group[1].PMUX[4].reg = PORT_PMUX_PMUXE_B; // note PMUXE for EVEN pin
  while (ADC->STATUS.bit.SYNCBUSY);

  configure_adc(ADC_PIN_POS, ADC_PIN_NEG, ADC_REFCTRL_REFSEL_INT1V);

  // Last Step - Enable ADC
  ADC->CTRLA.reg = ADC_CTRLA_ENABLE;
  while (ADC->STATUS.bit.SYNCBUSY);

}

void cryo_configure_adc(
    uint16_t pin_pos, uint16_t pin_neg, uint16_t reference) {

  ADC->INPUTCTRL.reg =
    // Gain control
    ADC_INPUTCTRL_GAIN_4X |
    // Select MUXNEG input
    pin_neg | // Pin2 == PB08 == A1
    // Select MUXPOS inut
    pin_pos; // Pin3 == PB09 == A2
  while (ADC->STATUS.bit.SYNCBUSY);

  // // Step 2 - Configure ADC reference
  ADC->REFCTRL.reg = 
    // enable reference buffer offset compensation
    ADC_REFCTRL_REFCOMP | 
    // select internal 1.0V voltage reference
    reference;
  while (ADC->STATUS.bit.SYNCBUSY);
  
  // Step 5 - Set sample count
  ADC->AVGCTRL.reg = 
    // One sample per trigger
    ADC_AVGCTRL_SAMPLENUM_1024;
  while (ADC->STATUS.bit.SYNCBUSY);

  // Configure output PIN
  PORT->Group[0].DIRSET.reg = PORT_PA02;
  PORT->Group[0].PINCFG[2].reg |= PORT_PINCFG_PMUXEN;
  PORT->Group[0].PMUX[1].reg = PORT_PMUX_PMUXE_B; // MUX[1] is MUX[N] where N = 2*n for even 'n'

}

int32_t cryo_read_adc() {
  
  // Read ADC value
  int16_t adc_conversion;
  // - Trigger conversion
  ADC->SWTRIG.reg = ADC_SWTRIG_START;
  while (ADC->STATUS.bit.SYNCBUSY);

  // int16_t adc_conversion;
  // // wait until ready
  while (ADC->INTFLAG.bit.RESRDY == 0) {} 
  // store value
  adc_conversion = ADC->RESULT.reg;

  return adc_conversion;

}