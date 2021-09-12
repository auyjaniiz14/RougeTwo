/*
 * ESP32 BMS CAN communication to VCU Unit
 * Auther : Rangson Pluemkamon
 * Version : 1.00
 * Date create/modified : 04-08-21
 */

#include "driver/gpio.h"
#include "driver/can.h"

uint16_t sys_voltage, sys_current, cell_max_vol, cell_min_vol;
uint16_t cell_ave_vol, cell_dev_vol, chg_current_limit, dchg_current_limit;
uint8_t sys_soc, can_counter, max_vol_cell_num, max_vol_grp_num, min_vol_cell_num, min_vol_grp_num;
int sys_status, chg_link, chg_indication, err_level, bms_self_check, power_off_rqst, insu_fade;
uint8_t cell_max_temp, max_temp_cell_num, max_temp_grp_num, cell_min_temp, min_temp_cell_num, min_temp_grp_num;
uint8_t cell_ave_temp, cell_dev_temp;

void setup(void) {
  Serial.begin(115200);
  setup_can_driver();
}

void setup_can_driver(void){
  can_general_config_t g_config = { // Create g_config for setup CAN Parameter
    .mode = CAN_MODE_NORMAL,
    .tx_io = GPIO_NUM_26, // CAN TX Pin GPIO_26
    .rx_io = GPIO_NUM_27, // CAN RX Pin GPIO_27
    .clkout_io = ((gpio_num_t) - 1),
    .bus_off_io = ((gpio_num_t) - 1),
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

void loop(void) {
  sys_voltage = random(4000,8000);
  sys_current = random(250,500);
  sys_soc = map(sys_voltage,4000,8000,125,250);
  periodic_message();
}

/*
 * Create function for CAN message function
 * List of message state : BMS_State, Cell_Vol State, Cell Temperature State, BattSysState
 */

void periodic_message(void);
void bms_state(void);
void cell_vol_1(void);
void cell_vol_2(void);
void cell_temperature(void);

void bms_state(){
  can_message_t txmessage;
  txmessage.identifier = 0x10FF50F4; // message id for bms state
  txmessage.flags = CAN_MSG_FLAG_EXTD;
  txmessage.data_length_code = 8; // 8 Byte Data Send
  txmessage.data[0] = sys_voltage >> 8;
  txmessage.data[1] = sys_voltage;
  txmessage.data[2] = sys_current >> 8;
  txmessage.data[3] = sys_current;
  txmessage.data[4] = sys_soc;
  txmessage.data[5] = sys_status >> 4;
  txmessage.data[5] = chg_link >> 2;
  txmessage.data[5] = chg_indication;
  txmessage.data[6] = err_level >> 6;
  txmessage.data[6] = bms_self_check >> 4;
  txmessage.data[6] = power_off_rqst >> 2;
  txmessage.data[6] = insu_fade;
  txmessage.data[7] = can_counter;
  
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
  txmessage.identifier = 0x18FF51F4; // message id for cell vol 1
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
  txmessage.identifier = 0x18FF52F4; // message id for cell vol 2
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
  txmessage.identifier = 0x18FF53F4; // message id for cell temp
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

void periodic_message(){
  bms_state(); cell_vol_1(); cell_vol_2(); cell_temperature();
  delay(100);
  bms_state();
  delay(100);
  bms_state(); cell_vol_1(); cell_vol_2(); cell_temperature();
  delay(100);
  bms_state();
  delay(100);
}
