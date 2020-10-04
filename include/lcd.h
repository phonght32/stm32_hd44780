// MIT License

// Copyright (c) 2020 thanhphong98

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.


#ifndef _LCD_H_
#define _LCD_H_

#ifdef __cplusplus 
extern "C" {
#endif

#include "stm_err.h"
#include "driver/gpio.h"
#include "driver/i2c.h"

typedef struct lcd *lcd_handle_t;

typedef enum {
	LCD_SIZE_16_2 = 0,
	LCD_SIZE_16_4,
	LCD_SIZE_20_4,
	LCD_SIZE_128_64,
} lcd_size_t;

typedef enum {
	LCD_DRIVER_HD44780,
	LCD_DRIVER_ST9720,
	LCD_DRIVER_PCF_8574,
} lcd_driver_t;

typedef enum {
	LCD_COMM_MODE_4BIT = 0,
	LCD_COMM_MODE_8BIT,
	LCD_COMM_MODE_SERIAL,
} lcd_comm_mode_t;

typedef struct {
	gpio_port_t		gpio_port_rs;
	gpio_num_t		gpio_num_rs;
	gpio_port_t		gpio_port_rw;
	gpio_num_t		gpio_num_rw;
	gpio_port_t		gpio_port_en;
	gpio_num_t		gpio_num_en;
	gpio_port_t		gpio_port_d0;
	gpio_num_t		gpio_num_d0;
	gpio_port_t		gpio_port_d1;
	gpio_num_t		gpio_num_d1;
	gpio_port_t		gpio_port_d2;
	gpio_num_t		gpio_num_d2;
	gpio_port_t		gpio_port_d3;
	gpio_num_t		gpio_num_d3;
	gpio_port_t		gpio_port_d4;
	gpio_num_t		gpio_num_d4;
	gpio_port_t		gpio_port_d5;
	gpio_num_t		gpio_num_d5;
	gpio_port_t		gpio_port_d6;
	gpio_num_t		gpio_num_d6;
	gpio_port_t		gpio_port_d7;
	gpio_num_t		gpio_num_d7;
	i2c_num_t		i2c_num;
	i2c_pins_pack_t	i2c_pins_pack;
} lcd_pin_t;

typedef struct {
	lcd_size_t 			size;
	lcd_driver_t 		driver;
	lcd_comm_mode_t 	mode;
	lcd_pin_t			pin;
} lcd_cfg_t;

lcd_handle_t lcd_init(lcd_cfg_t *config);
stm_err_t lcd_clear(lcd_handle_t handle);
stm_err_t lcd_home(lcd_handle_t handle);
stm_err_t lcd_write_string(lcd_handle_t handle, uint8_t *str);
stm_err_t lcd_goto(lcd_handle_t handle, uint8_t x, uint8_t y);
stm_err_t lcd_cursor_shift(lcd_handle_t handle, int step, bool dir);
 

#ifdef __cplusplus
}
#endif

#endif /* _LCD_H_ */

