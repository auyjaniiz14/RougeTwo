#ifndef DATACONVERSION_H
#define DATACONVERSION_H

#include <Arduino.h>
#include <stdint.h>

uint16_t sys_voltage_conv(uint16_t _value);
uint16_t sys_current_conv(uint16_t _value);
uint8_t sys_soc_conv(uint8_t _value);

uint16_t chg_current_limit_conv(uint16_t _value);
uint16_t dchg_current_limit_conv(uint16_t _value);

uint8_t cell_max_temp_conv(uint8_t _value);
uint8_t cell_min_temp_conv(uint8_t _value);
uint8_t cell_ave_temp_conv(uint8_t _value);

uint16_t max_chg_current_conv(uint16_t _value);

uint16_t I_ch_max_cell_spec_conv(uint16_t _value);

uint16_t acc_charge_conv(uint16_t _value);
uint16_t acc_discharge_conv(uint16_t _value);
uint32_t master_timer_conv(uint32_t _value);

uint16_t soh_conv(uint16_t _value);



#endif
