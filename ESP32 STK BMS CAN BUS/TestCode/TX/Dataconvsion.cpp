#include <Arduino.h>
#include <stdint.h>
#include "Dataconversion.h"

/* Data conversion equation
 * E = (N * x) + C
 * by E = Engineering value -> true value after calculation
 *    N = Effective value -> measurement value
 *    x = Factor -> factor for data conversion
 *    C = offsetust -> offsetust value
 * Then N = (E - C)/x -> value transmittion
 */

uint16_t sys_voltage_conv(uint16_t _value){
  double factor = 0.1;
  int offset = 0;
  return (_value - offset)/factor;
}

uint16_t sys_current_conv(uint16_t _value){
  double factor = 0.1;
  int offset = -500;
  return (_value - offset)/factor;
}

uint8_t sys_soc_conv(uint8_t _value){
  double factor = 0.4;
  int offset = 0;
  return (_value - offset)/factor;
}

uint16_t chg_current_limit_conv(uint16_t _value){
  double factor = 0.1;
  int offset = -500;
  return (_value - offset)/factor;
}

uint16_t dchg_current_limit_conv(uint16_t _value){
  double factor = 0.1;
  int offset = -500;
  return (_value - offset)/factor;
}

uint8_t cell_max_temp_conv(uint8_t _value){
  double factor = 1;
  int offset = -40;
  return (_value - offset)/factor;
}

uint8_t cell_min_temp_conv(uint8_t _value){
  double factor = 1;
  int offset = -40;
  return (_value - offset)/factor;
}

uint8_t cell_ave_temp_conv(uint8_t _value){
  double factor = 1;
  int offset = -40;
  return (_value - offset)/factor;
}

uint16_t max_chg_current_conv(uint16_t _value){
  double factor = 0.1;
  int offset = -500;
  return (_value - offset)/factor;
}

uint16_t I_ch_max_cell_spec_conv(uint16_t _value){
  double factor = 0.1;
  int offset = -500;
  return (_value - offset)/factor;
}

uint16_t acc_charge_conv(uint16_t _value){
  double factor = 0.5;
  int offset = 0;
  return (_value - offset)/factor;
}

uint16_t acc_discharge_conv(uint16_t _value){
  double factor = 0.5;
  int offset = 0;
  return (_value - offset)/factor;
}

uint32_t master_timer_conv(uint32_t _value){
  double factor = 0.1;
  int offset = 0;
  return (_value - offset)/factor;
}


uint16_t soh_conv(uint16_t _value){
  double factor = 0.1;
  int offset = 0;
  return (_value - offset)/factor;
}
