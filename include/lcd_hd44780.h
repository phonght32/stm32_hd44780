// MIT License

// Copyright (c) 2020 phonght32

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

typedef struct lcd_hd44780 *lcd_hd44780_handle_t;	/* LCD handle structure */

typedef enum {
	LCD_HD44780_SIZE_16_2 = 0,						/*!< LCD size 16x2 */
	LCD_HD44780_SIZE_16_4,							/*!< LCD size 16x4 */
	LCD_HD44780_SIZE_20_4,							/*!< LCD size 20x4 */
} lcd_hd44780_size_t;

typedef enum {
	LCD_HD44780_COMM_MODE_4BIT = 0,					/*!< Communicate with LCD over 4bit data mode */
	LCD_HD44780_COMM_MODE_8BIT,						/*!< Communicate with LCD over 8bit data mode */
	LCD_HD44780_COMM_MODE_SERIAL,					/*!< Communicate with LCD over serial data mode */
} lcd_hd44780_comm_mode_t;

typedef struct {
	gpio_port_t		gpio_port_rs;					/*!< GPIO Port RS */
	gpio_num_t		gpio_num_rs;					/*!< GPIO Num RS */
	gpio_port_t		gpio_port_rw;					/*!< GPIO Port RW */
	gpio_num_t		gpio_num_rw;					/*!< GPIO Num RW */
	gpio_port_t		gpio_port_en;					/*!< GPIO Port EN */
	gpio_num_t		gpio_num_en;					/*!< GPIO Num EN */
	gpio_port_t		gpio_port_d0;					/*!< GPIO Port D0 */
	gpio_num_t		gpio_num_d0;					/*!< GPIO Num D0 */
	gpio_port_t		gpio_port_d1;					/*!< GPIO Port D1 */
	gpio_num_t		gpio_num_d1;					/*!< GPIO Num D1 */
	gpio_port_t		gpio_port_d2;					/*!< GPIO Port D2 */
	gpio_num_t		gpio_num_d2;					/*!< GPIO Num D2 */
	gpio_port_t		gpio_port_d3;					/*!< GPIO Port D3 */
	gpio_num_t		gpio_num_d3;					/*!< GPIO Num D3 */
	gpio_port_t		gpio_port_d4;					/*!< GPIO Port D4 */
	gpio_num_t		gpio_num_d4;					/*!< GPIO Num D4 */
	gpio_port_t		gpio_port_d5;					/*!< GPIO Port D5 */
	gpio_num_t		gpio_num_d5;					/*!< GPIO Num D5 */
	gpio_port_t		gpio_port_d6;					/*!< GPIO Port D6 */
	gpio_num_t		gpio_num_d6;					/*!< GPIO Num D6 */
	gpio_port_t		gpio_port_d7;					/*!< GPIO Port D7 */
	gpio_num_t		gpio_num_d7;					/*!< GPIO Num D7 */
	i2c_num_t		i2c_num;						/*!< I2C Num for serial mode*/
	i2c_pins_pack_t	i2c_pins_pack;					/*!< I2C Pins Pack for serial mode */
} lcd_hd44780_pin_t;

typedef struct {
	lcd_hd44780_size_t 			size;				/*!< LCD size */
	lcd_hd44780_comm_mode_t 	mode;				/*!< LCD communicate mode */
	lcd_hd44780_pin_t			pin;				/*!< LCD pins */
} lcd_hd44780_cfg_t;

/*
 * @brief   Initialize Liquid-Crystal Display (LCD).
 * @param   config Struct pointer.
 * @return
 *      - LCD handle structure: Success.
 *      - 0: Fail.
 */
lcd_hd44780_handle_t lcd_hd44780_init(lcd_hd44780_cfg_t *config);

/*
 * @brief   Clear LCD screen.
 * @param   handle Handle structure.
 * @return
 *      - STM_OK:   Success.
 *      - STM_FAIL: Fail.
 */
stm_err_t lcd_hd44780_clear(lcd_hd44780_handle_t handle);

/*
 * @brief   Set LCD cursor to home.
 * @param   handle Handle structure.
 * @return
 *      - STM_OK:   Success.
 *      - STM_FAIL: Fail.
 */
stm_err_t lcd_hd44780_home(lcd_hd44780_handle_t handle);

/*
 * @brief   Display string.
 * @param   handle Handle structure.
 * @param 	str String display.
 * @return
 *      - STM_OK:   Success.
 *      - STM_FAIL: Fail.
 */
stm_err_t lcd_hd44780_write_string(lcd_hd44780_handle_t handle, uint8_t *str);
 

#ifdef __cplusplus
}
#endif

#endif /* _LCD_H_ */

