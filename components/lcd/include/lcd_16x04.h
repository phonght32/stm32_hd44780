#ifndef _LCD_16X04_H_
#define  _LCD_16X04_H_
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
#include "i2c_protocol.h"
#define LCD_ADDRESS 0x27
// command
#define LCD_BASIC 0x30
#define LCD_CLEAR 0x01
#define LCD_CURSOR_BACK 0x02
#define LCD_CURSOR_NEXT_CHAR 0x06
#define LCD_DISPLAY_ON_CURSOR_OFF 0x0C
#define LCD_DISPLAY_ON_CURSOR_ON 0x0E
#define LCD_CURSOR_TURN_ROW1 0x80
#define LCD_CURSOR_TURN_ROW2 0xC0
#define LCD_2_ROWS_5x7SIZE 0x28
#define LCD_SCROLL_LEFT 0x18
#define LCD_SCROLL_RIGHT 0x1C

void LCD_INIT();
void LCD_goto_XY(int X, int Y);
BYTE* lcd_set_command(BYTE command);
BYTE* lcd_set_char (BYTE character);
void i2c_send_command(BYTE command);
void i2c_send_char(BYTE character);
void i2c_send_string(char* string_data);
#ifdef __cplusplus
}
#endif

#endif /* RS485_TRANSPORT_H_ */