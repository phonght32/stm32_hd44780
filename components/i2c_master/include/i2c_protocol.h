#ifndef _I2C_PROTOCOL_H_
#define  _I2C_PROTOCOL_H_
#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/i2c.h"


#define MASTER_FREQUENTLY  100000
#define MASTER_SDA_IO      4
#define MASTER_SCL_IO      19
#define I2C_PORT_NUM       1
#define I2C_MASTER_BUFFER_TX 0
#define I2C_MASTER_BUFFER_RX 0
#define ACK_EN 1
#define ACK_DIS 0
typedef uint8_t BYTE;
esp_err_t i2c_master_init();
esp_err_t master_write_byte(BYTE SLAY_ADDRESS,uint8_t* data, int data_size);

#ifdef __cplusplus
}
#endif

#endif /* RS485_TRANSPORT_H_ */