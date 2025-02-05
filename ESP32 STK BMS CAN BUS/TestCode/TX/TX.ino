/*
 * ESP32 BMS CAN communication to VCU Unit
 * Author : Rangson Pluemkamon
 * Version : 1.00
 * Date create/modified : 04-08-21
 */

#include <Arduino.h>
#include <stdint.h>
#include "Dataconversion.h"
#include "driver/gpio.h" // only use in board ESP32
#include "driver/can.h" // only use in board ESP32


// BMS State data assign
uint16_t sys_voltage, sys_current; // 16-bit data
uint8_t can_counter; // 8-bit variables
int sys_soc, sys_status, chg_link, chg_indication, err_level, bms_self_check, power_off_rqst, insu_fade; // 1-4-bit data

// Cell_vol_1, Cell_Vol_2 data assign
uint16_t cell_max_vol, cell_min_vol, cell_ave_vol, cell_dev_vol, chg_current_limit, dchg_current_limit;; // 16-bit data
uint8_t max_vol_cell_num, max_vol_grp_num, min_vol_cell_num, min_vol_grp_num; // 8-bit data

// Cell Temperature data assign
uint8_t cell_max_temp, max_temp_cell_num, max_temp_grp_num, cell_min_temp, min_temp_cell_num, min_temp_grp_num, cell_ave_temp, cell_dev_temp; // 8-bit data

// Battery Sys State data assign
uint16_t max_chg_current; // 16-bit data
int cell_vol_high, cell_vol_low, tem_high, tem_low; // byte 1 data
int sum_vol_high, sum_vol_low, dis_current_over, chg_current_over; // byte 2 data
int soc_high, soc_low, det_temp, det_cell_vol; // byte 3 data
int bms_commu_fault, vcu_timeout_malf, sum_vol_diff_malf, port_temp_high; // byte 4 data
int st_pos_relay, st_chg_relay, st_neg_relay, st_prechg_relay, dc_dc_relay; // byte 5 data
int air_pump_relay, port_temp_relay; // byte 6 data

// Battery Spec data assign
uint16_t bms_serial_nr, v_max_cell_spec, I_ch_max_cell_spec; // 16-bit data
uint8_t batt_vendor_nr, t_max_cell_spec; // 8-byte data

// Accumulate Spec data assign
uint32_t master_timer; // 32-bit data
uint16_t acc_charge, acc_discharge; // 16-bit data

// SOH data assign
uint16_t soh_data;

// Assign message id for can transmitter
uint32_t bms_state_id = 0x10FF50F4;
uint32_t cell_vol_1_id = 0x18FF51F4;
uint32_t cell_vol_2_id = 0x18FF52F4;
uint32_t cell_temperature_id = 0x18FF53F4;
uint32_t battery_sys_state_id = 0x18FF54F4;
uint32_t battery_spec_id = 0x18FF60F4;
uint32_t accumulate_data_id = 0x18F009A0;
uint32_t soh_id = 0x18F001A0;



void setup_can_driver(){
  can_general_config_t g_config = { // Create g_config for setup CAN Parameter
    .mode = CAN_MODE_NORMAL,
    .tx_io = GPIO_NUM_26, // CAN TX Pin GPIO_26
    .rx_io = GPIO_NUM_27, // CAN RX Pin GPIO_27
    .clkout_io = ((gpio_num_t)CAN_IO_UNUSED),
    .bus_off_io = ((gpio_num_t)CAN_IO_UNUSED),
    .tx_queue_len = 5,
    .rx_queue_len = 5,
    .alerts_enabled = CAN_ALERT_ALL,
    .clkout_divider = 0
  };
  can_timing_config_t t_config = CAN_TIMING_CONFIG_250KBITS(); // Set baud rate 250Kbps
  can_filter_config_t f_config = CAN_FILTER_CONFIG_ACCEPT_ALL();

  if (can_driver_install(&g_config, &t_config, &f_config) != ESP_OK) {
    Serial.println("Failed to install driver");
    return;
  }

  if (can_start() != ESP_OK) {
    Serial.println("Failed to start driver");
    return;
  }
}

void setup(void) {
  Serial.begin(115200);
  setup_can_driver();
  /*
   * Initial data value
   */
  sys_voltage = sys_voltage_conv(72); // voltage 72 v
  sys_current = sys_current_conv(32); // current 63 A
  sys_soc = sys_soc_conv(60); // Soc = 60%

  cell_max_vol = 3200; // 3200 mV
  cell_min_vol = 3200; // 3200 mV
  cell_ave_vol = 3200; // 3200 mV
  
  chg_current_limit = chg_current_limit_conv(72); // 72 A
  dchg_current_limit = dchg_current_limit_conv(100); // 100 A

  cell_max_temp = cell_max_temp_conv(70); // 70
  cell_min_temp = cell_min_temp_conv(65); // 35
  cell_ave_temp = cell_ave_temp_conv(40); // 40

  max_chg_current = max_chg_current_conv(60); // 60 A 

  I_ch_max_cell_spec = I_ch_max_cell_spec_conv(30); // 30 A

  acc_charge = acc_charge_conv(40000);
  acc_discharge = acc_discharge_conv(40000);

  soh_data = soh_conv(95); // Soh = 95%
}

void loop(void) {
  periodic_message2();
}

/*
 * Create function for CAN message function
 * List of message state : BMS_State, Cell_Vol State, Cell Temperature State, BattSysState
 */

void periodic_message(void); // Send periodic can message by different state cycle
void periodic_message2(void);
void bms_state(void);
void cell_vol_1(void);
void cell_vol_2(void);
void cell_temperature(void);
void battery_sys_state(void);
void battery_spec(void);
void accumulate_data(void);
void soh(void);

void bms_state(){
  byte byte5 = 0b10010100; /* chg_indication, chg_link, sys_status
  byte5 data default is 0b00000000
  bit4/bit3/bit2/bit1:Battery system Status
  0000： Get Ready 
  0001： Wait power up  
  0010： Prepare_chage  
  0011： Prepare_chage Fail  
  0100： Power On Enabled  
  0101： Power Off
  bit6/bit5:Charge link Status
  00： no 
  01： charge link OK
  11： useless 
  bit8/bit7:Charge  Status
  00： normal    (no charge)
  01： charging  
  10： charge full 
  11： charge error 
  */
  byte byte6 = 0b01100001; /* insu_fade, power_off_rqst, bms_self_check, err_level
  byte6 data default is 0b00000000
  bit2/bit1:Error level of battery system
  00： Norma    
  01： Warning  One_LV 
  10： Warning  Two_LV
  11： Alarm    Third_LV
  bit4/bit3:BMS self-check
  00： Norma
  01： BMS_Err
  11： useless
  bit6/bit5:BMS request
  00： Normal 
  01： Request  HV Power Off（start timing of discharge protection） 
  10： HV Power Off  
  11： useless
  bit8/bit7: Insulation Error
  00： Normal 
  01： Warning  <500Ω/V，
  10： Alarm    <100Ω/V
  */
  can_message_t txmessage;
  txmessage.identifier = bms_state_id; // message id for bms state
  txmessage.flags = CAN_MSG_FLAG_EXTD;
  txmessage.data_length_code = 8; // 8 Byte Data Send
  txmessage.data[0] = sys_voltage >> 8;
  txmessage.data[1] = sys_voltage;
  txmessage.data[2] = sys_current >> 8;
  txmessage.data[3] = sys_current;
  txmessage.data[4] = sys_soc; // Soc = N*x + C by x = 0.4
  txmessage.data[5] = byte5;
  txmessage.data[6] = byte6;
  txmessage.data[7] = can_counter;
  // Send can message
  if (can_transmit(&txmessage, pdMS_TO_TICKS(1000)) == ESP_OK) {
    Serial.println("message queued for transmission");
  } else {
    Serial.println("Failed to queue txmessage for transmission");
  }
  can_counter ++;
  delay(1);
}

void cell_vol_1(){
  can_message_t txmessage;
  txmessage.identifier = cell_vol_1_id; // message id for cell vol 1
  txmessage.flags = CAN_MSG_FLAG_EXTD;
  txmessage.data_length_code = 8; // 8 Byte Data Send
  txmessage.data[0] = cell_max_vol >> 8;
  txmessage.data[1] = cell_max_vol;
  txmessage.data[2] = cell_min_vol >> 8;
  txmessage.data[3] = cell_min_vol;
  txmessage.data[4] = max_vol_cell_num;
  txmessage.data[5] = max_vol_grp_num;
  txmessage.data[6] = min_vol_cell_num;
  txmessage.data[7] = min_vol_grp_num;
  
  if (can_transmit(&txmessage, pdMS_TO_TICKS(1000)) == ESP_OK) {
    Serial.println("message queued for transmission");
  } else {
    Serial.println("Failed to queue txmessage for transmission");
  }
  delay(1);
}

void cell_vol_2(){
  can_message_t txmessage;
  txmessage.identifier = cell_vol_2_id; // message id for cell vol 2
  txmessage.flags = CAN_MSG_FLAG_EXTD;
  txmessage.data_length_code = 8; // 8 Byte Data Send
  txmessage.data[0] = cell_ave_vol >> 8;
  txmessage.data[1] = cell_ave_vol;
  txmessage.data[2] = cell_dev_vol >> 8;
  txmessage.data[3] = cell_dev_vol;
  txmessage.data[4] = chg_current_limit >> 8;
  txmessage.data[5] = chg_current_limit;
  txmessage.data[6] = dchg_current_limit >> 8;
  txmessage.data[7] = dchg_current_limit;
  
  if (can_transmit(&txmessage, pdMS_TO_TICKS(1000)) == ESP_OK) {
    Serial.println("message queued for transmission");
  } else {
    Serial.println("Failed to queue txmessage for transmission");
  }
  delay(1);
}

void cell_temperature(){
  can_message_t txmessage;
  txmessage.identifier = cell_temperature_id; // message id for cell temp
  txmessage.flags = CAN_MSG_FLAG_EXTD;
  txmessage.data_length_code = 8; // 8 Byte Data Send
  txmessage.data[0] = cell_max_temp;
  txmessage.data[1] = max_temp_cell_num;
  txmessage.data[2] = max_temp_grp_num;
  txmessage.data[3] = cell_min_temp;
  txmessage.data[4] = min_temp_cell_num;
  txmessage.data[5] = min_temp_grp_num;
  txmessage.data[6] = cell_ave_temp;
  txmessage.data[7] = cell_dev_temp;

  if (can_transmit(&txmessage, pdMS_TO_TICKS(1000)) == ESP_OK) {
    Serial.println("message queued for transmission");
  } else {
    Serial.println("Failed to queue txmessage for transmission");
  }
  delay(1);
}

void battery_sys_state(){
  byte byte1 = 0b01101001; /* TempLow, TempHigh, CellVolLow, CellVolHigh
  byte1 data default is 0b00000000
  bit2/bit1 : CellVolHigh
  00  Normal
  01  ErrLevel One :Battrey Warning
  10  ErrLevel Two : Battery Warning ， VCU Reduced feelback power
  11  ErrLevel Three : Battery Alarm  and   Charging Power failure
  bit4/bit3 : CellVolLow
  00  Normal
  01  ErrLevel One : Battery Warning
  10  ErrLevel Two : Battery Warning ， VCU Reduced discharge power
  11  ErrLevel Three : Battery Alarm  and   Discharging Power failure
  bit6/bit5 : TempHigh
  00  Normal
  01  ErrLevel One : Battery Warning
  10  ErrLevel Two : Battery Warning ， VCU Reduced power
  11  ErrLevel Three : Battery Alarm  and  Charging-discharging  Power failure
  bit8/bit7 : TempLow
  00  Normal
  01  ErrLevel One : Battery Warning
  10  ErrLevel Two : Battery Warning ， VCU Reduced power
  11  ErrLevel Three : Battery Alarm  and  Charging-discharging  Power failure
  */
  byte byte2 = 0b01101001; /*
  byte2 data default is 0b00000000 
  bit2/bit1 : SumVolHigh
  00  Normal
  01  ErrLevel One :Battrey Warning
  10  ErrLevel Two : Battery Warning ， VCU Reduced feelback power
  11  ErrLevel Three : Battery Alarm  and   Charging Power failure"
  bit4/bit3 : SumVolLow
  00  Normal
  01  ErrLevel One : Battery Warning
  10  ErrLevel Two : Battery Warning ， VCU Reduced discharge power
  11  ErrLevel Three : Battery Alarm  and   Discharging Power failure"
  bit6/bit5 : DisCurrent Over
  00  Normal
  01  ErrLevel One : Battery Warning
  10  ErrLevel Two : Battery Warning ， VCU Reduced discharge power
  11  ErrLevel Three : Battery Alarm  and  VCU Reduced discharge power"
  bit8/bit7 : ChaCurrent Over
  00  Normal
  01  ErrLevel One : Battery Warning
  10  ErrLevel Two : Battery Warning ， VCU Reduced feelback power
  11  ErrLevel Three : Battery Alarm  and  VCU Reduced feelback power"
  */
  byte byte3 = 0b01101001; /*
  byte3 data default is 0b00000000
  bit2/bit1 : SOC High
  00  Normal
  01  ErrLevel One 
  10  ErrLevel Two 
  11  ErrLevel Three "
  bit4/bit3 : SOC Low
  00  Normal
  01  ErrLevel One : Battery Warning
  10  ErrLevel Two : Battery Warning ， VCU Reduced discharge power
  11  ErrLevel Three : Battery Alarm  and  VCU Reduced discharge power"
  bit6/bit5 : Det_Tempe
  00  Normal
  01  ErrLevel One 
  10  ErrLevel Two 
  11  ErrLevel Three "
  bit8/bit7 : Det CellVol
  00  Normal
  01  ErrLevel One 
  10  ErrLevel Two 
  11  ErrLevel Three "
  */
  byte byte4 = 0b01101001; /* BMS/Communiation Fault, VCU timeout malf, Sum Voltage diff malf, Port Temp High
  byte4 data default is 0b00000000
  bit2/bit1 : BMS/Communication Fault
  00  Normal
  01  ErrLevel One : Battery Warning
  10  ErrLevel Two : Battery Warning ， VCU Reduced power
  11  ErrLevel Three : Battery Alarm  and  Charging-discharging  Power failure"
  bit4/bit3 : VCU timeout malfunction
  00  Normal
  01  ErrLevel One : Battery Warning
  10  ErrLevel Two : Battery Warning ， VCU Reduced power
  11  ErrLevel Three : Battery Alarm  and  Charging-discharging  Power failure"
  bit6/bit5 : Sum Voltage difference malfunction
  00  Normal
  01  ErrLevel One : Battery Warning
  10  ErrLevel Two : Battery Warning ， VCU Reduced power
  11  ErrLevel Three : Battery Alarm  and  Charging-discharging  Power failure"
  bit8/bit7 : Port Temp High
  00  Normal
  01  ErrLevel One : Battery Warning
  10  ErrLevel Two : Battery Warning ， VCU Reduced power
  11  ErrLevel Three : Battery Alarm  and  Charging-discharging  Power failure"
  */
  byte byte5 = 0b01101001; /* St_Pos_Relay, St_chargeRelay, St_neg_Relay, DC/DC Relay
  byte5 data default is 0b00000000
  bit2/bit1 : St_Pos_Relay
  00 ：open    
  01 ：closed  
  10 ：Error    Relay failure
  11 ；useless "
  bit4/bit3 : St_chargeRelay
  00 ：open    
  01 ：closed  
  10 ：Error    Relay failure
  11 ；useless "
  bit6/bit5 : St_neg_Relay
  00 ：open    
  01 ：closed  
  10 ：Error    Relay failure
  11 ；useless "
  bit8/bit7 : DC/DC_Relay
  00 ：open    
  01 ：closed  
  10 ：Error    Relay failure
  11 ；useless "
  */
  byte byte6 = 0b01101001; /* Air Pump Relay, Max Charge Current
  byte6 data default is 0b00000000
  bit2/bit1 : Air Pump Relay
  00 ：open    
  01 ：closed  
  10 ：Error    Relay failure
  11 ；useless "
  bit4/bit3 : Max Charge Current
  00 ：normal   
  01 ：Temperature Sensor  Malfunction
  10 ：useless
  11 ；useless "
  */
  can_message_t txmessage;
  txmessage.identifier = battery_sys_state_id; // message id for Battery Sys State
  txmessage.flags = CAN_MSG_FLAG_EXTD;
  txmessage.data_length_code = 8; // 8 Byte Data Send
  txmessage.data[0] = byte1;
  txmessage.data[1] = byte2;
  txmessage.data[2] = byte3;
  txmessage.data[3] = byte4;
  txmessage.data[4] = byte5;
  txmessage.data[5] = byte6;
  txmessage.data[6] = max_chg_current >> 8;
  txmessage.data[7] = max_chg_current;

  if (can_transmit(&txmessage, pdMS_TO_TICKS(1000)) == ESP_OK) {
    Serial.println("message queued for transmission");
  } else {
    Serial.println("Failed to queue txmessage for transmission");
  }
  delay(1);
}

void battery_spec(){
  can_message_t txmessage;
  txmessage.identifier = battery_spec_id; // message id for Battery Spec
  txmessage.flags = CAN_MSG_FLAG_EXTD;
  txmessage.data_length_code = 8; // 8 Byte Data Send
  txmessage.data[0] = batt_vendor_nr;
  txmessage.data[1] = bms_serial_nr >> 8;
  txmessage.data[2] = bms_serial_nr;
  txmessage.data[3] = v_max_cell_spec >> 8;
  txmessage.data[4] = v_max_cell_spec;
  txmessage.data[5] = t_max_cell_spec;
  txmessage.data[6] = I_ch_max_cell_spec >> 8;
  txmessage.data[7] = I_ch_max_cell_spec;

  if (can_transmit(&txmessage, pdMS_TO_TICKS(1000)) == ESP_OK) {
    Serial.println("message queued for transmission");
  } else {
    Serial.println("Failed to queue txmessage for transmission");
  }
  delay(1);
}

void accumulate_data(){
  can_message_t txmessage;
  txmessage.identifier = accumulate_data_id; // message id for Battery Spec
  txmessage.flags = CAN_MSG_FLAG_EXTD;
  txmessage.data_length_code = 8; // 8 Byte Data Send
  txmessage.data[0] = acc_charge >> 8;
  txmessage.data[1] = acc_charge;
  txmessage.data[2] = acc_discharge >> 8;
  txmessage.data[3] = acc_discharge;
  txmessage.data[4] = master_timer >> 24;
  txmessage.data[5] = master_timer >> 16;
  txmessage.data[6] = master_timer >> 8;
  txmessage.data[7] = master_timer;

  if (can_transmit(&txmessage, pdMS_TO_TICKS(1000)) == ESP_OK) {
    Serial.println("message queued for transmission");
  } else {
    Serial.println("Failed to queue txmessage for transmission");
  }
  delay(1);
}

void soh(){
  byte byte1 = 0b00000000;
  byte byte2 = 0b00000000;
  byte byte3 = 0b00000000;
  byte byte4 = 0b00000000;
  byte byte7 = 0b00000000;
  byte byte8 = 0b00000000;
  can_message_t txmessage;
  txmessage.identifier = soh_id; // message id for Battery Spec
  txmessage.flags = CAN_MSG_FLAG_EXTD;
  txmessage.data_length_code = 8; // 8 Byte Data Send
  txmessage.data[0] = byte1;
  txmessage.data[1] = byte2;
  txmessage.data[2] = byte3;
  txmessage.data[3] = byte4;
  txmessage.data[4] = soh_data >> 8; // Soh data max 100
  txmessage.data[5] = soh_data; // 
  txmessage.data[6] = byte7;
  txmessage.data[7] = byte8;

  if (can_transmit(&txmessage, pdMS_TO_TICKS(1000)) == ESP_OK) {
    Serial.println("message queued for transmission");
  } else {
    Serial.println("Failed to queue txmessage for transmission");
  }
  delay(1);
}

void periodic_message(){
  bms_state(); cell_vol_1(); cell_vol_2(); cell_temperature(); battery_sys_state();
  delay(100);
  bms_state(); battery_sys_state();
  delay(100);
}

void periodic_message2(){
  bms_state(); cell_vol_1(); cell_vol_2(); cell_temperature(); battery_sys_state(); battery_spec(); accumulate_data(); soh();
  delay(100);
  bms_state(); battery_sys_state();
  delay(100);
  bms_state(); cell_vol_1(); cell_vol_2(); cell_temperature(); battery_sys_state();
  delay(100);
  bms_state(); battery_sys_state();
  delay(100);
  bms_state(); cell_vol_1(); cell_vol_2(); cell_temperature(); battery_sys_state();
  delay(100);
  bms_state(); battery_sys_state();
  delay(100);
  bms_state(); cell_vol_1(); cell_vol_2(); cell_temperature(); battery_sys_state();
  delay(100);
  bms_state(); battery_sys_state();
  delay(100);
  bms_state(); cell_vol_1(); cell_vol_2(); cell_temperature(); battery_sys_state();
  delay(100);
  bms_state(); battery_sys_state();
  delay(100);
  bms_state(); cell_vol_1(); cell_vol_2(); cell_temperature(); battery_sys_state(); soh();
  delay(100);
  bms_state(); battery_sys_state();
  delay(100);
}
