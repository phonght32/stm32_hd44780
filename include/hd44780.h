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


#ifndef _HD44780_H_
#define _HD44780_H_

#ifdef __cplusplus 
extern "C" {
#endif

#include "stm_err.h"
#include "driver/gpio.h"
#include "driver/i2c.h"

typedef struct hd44780 *hd44780_handle_t;	/* LCD handle structure */

typedef enum {
	HD44780_SIZE_16_2 = 0,						/*!< LCD size 16x2 */
	HD44780_SIZE_16_4,							/*!< LCD size 16x4 */
	HD44780_SIZE_20_4,							/*!< LCD size 20x4 */
	HD44780_SIZE_MAX,
} hd44780_size_t;

typedef enum {
	HD44780_COMM_MODE_4BIT = 0,					/*!< Communicate with LCD over 4bit data mode */
	HD44780_COMM_MODE_8BIT,						/*!< Communicate with LCD over 8bit data mode */
	HD44780_COMM_MODE_SERIAL,					/*!< Communicate with LCD over serial data mode */
	HD44780_COMM_MODE_MAX,
} hd44780_comm_mode_t;

typedef struct {
	int 				gpio_port_rs;				/*!< GPIO Port RS */
	int					gpio_num_rs;				/*!< GPIO Num RS */
	int					gpio_port_rw;				/*!< GPIO Port RW */
	int					gpio_num_rw;				/*!< GPIO Num RW */
	int					gpio_port_en;				/*!< GPIO Port EN */
	int					gpio_num_en;				/*!< GPIO Num EN */
	int					gpio_port_d0;				/*!< GPIO Port D0 */
	int					gpio_num_d0;				/*!< GPIO Num D0 */
	int					gpio_port_d1;				/*!< GPIO Port D1 */
	int					gpio_num_d1;				/*!< GPIO Num D1 */
	int					gpio_port_d2;				/*!< GPIO Port D2 */
	int					gpio_num_d2;				/*!< GPIO Num D2 */
	int					gpio_port_d3;				/*!< GPIO Port D3 */
	int					gpio_num_d3;				/*!< GPIO Num D3 */
	int					gpio_port_d4;				/*!< GPIO Port D4 */
	int					gpio_num_d4;				/*!< GPIO Num D4 */
	int					gpio_port_d5;				/*!< GPIO Port D5 */
	int					gpio_num_d5;				/*!< GPIO Num D5 */
	int					gpio_port_d6;				/*!< GPIO Port D6 */
	int					gpio_num_d6;				/*!< GPIO Num D6 */
	int					gpio_port_d7;				/*!< GPIO Port D7 */
	int					gpio_num_d7;				/*!< GPIO Num D7 */
	i2c_num_t			i2c_num;					/*!< I2C Num for serial mode*/
	i2c_pins_pack_t		i2c_pins_pack;				/*!< I2C Pins Pack for serial mode */
} hd44780_hardware_info_t;

typedef struct {
	hd44780_size_t 				size;			/*!< LCD size */
	hd44780_comm_mode_t 		mode;			/*!< LCD communicate mode */
	hd44780_hardware_info_t		hw_info;		/*!< LCD hardware information */
} hd44780_cfg_t;

/*
 * @brief   Initialize Liquid-Crystal Display (LCD).
 * @note:   This function only get I2C_NUM to handler communication, not
 *          configure I2C 's parameters. You have to self configure I2C before
 *          pass I2C into this function.
 * @param   config Struct pointer.
 * @return
 *      - LCD handle structure: Success.
 *      - 0: Fail.
 */
hd44780_handle_t hd44780_init(hd44780_cfg_t *config);

/*
 * @brief   Clear LCD screen.
 * @param   handle Handle structure.
 * @return
 *      - STM_OK:   Success.
 *      - Others: 	Fail.
 */
stm_err_t hd44780_clear(hd44780_handle_t handle);

/*
 * @brief   Set LCD cursor to home.
 * @param   handle Handle structure.
 * @return
 *      - STM_OK:   Success.
 *      - Others: 	Fail.
 */
stm_err_t hd44780_home(hd44780_handle_t handle);

/*
 * @brief   Display a character.
 * @param   handle Handle structure.
 * @param 	char Character.
 * @return
 *      - STM_OK:   Success.
 *      - Others:	Fail.
 */
stm_err_t hd44780_write_char(hd44780_handle_t handle, uint8_t chr);

/*
 * @brief   Display string.
 * @param   handle Handle structure.
 * @param 	str String display.
 * @return
 *      - STM_OK:   Success.
 *      - Others: 	Fail.
 */
stm_err_t hd44780_write_string(hd44780_handle_t handle, uint8_t *str);

/*
 * @brief   Display integer.
 * @param   handle Handle structure.
 * @param 	number Number as integer format.
 * @return
 *      - STM_OK:   Success.
 *      - Others: 	Fail.
 */
stm_err_t hd44780_write_int(hd44780_handle_t handle, int number);

/*
 * @brief   Display float.
 * @param   handle Handle structure.
 * @param 	number Number as float format.
 * @param 	precision Number of digit after decimal.
 * @return
 *      - STM_OK:   Success.
 *      - Others: 	Fail.
 */
stm_err_t hd44780_write_float(hd44780_handle_t handle, float number, uint8_t precision);

/*
 * @brief 	Move LCD's cursor to cordinate (x,y). 
 * @param   col Column position.
 * @param 	row Row position.
 * @return
 *      - STM_OK:   Success.
 *      - Others: 	Fail.
 */
stm_err_t hd44780_gotoxy(hd44780_handle_t handle, uint8_t col, uint8_t row);

/*
 * @brief   Shift cursor forward.
 * @param   handle Handle structure.
 * @param   step Number of step.
 * @return
 *      - STM_OK:   Success.
 *      - Others: 	Fail.
 */
stm_err_t hd44780_shift_cursor_forward(hd44780_handle_t handle, uint8_t step);

/*
 * @brief   Shift cursor backward.
 * @param   handle Handle structure.
 * @param   step Number of step.
 * @return
 *      - STM_OK:   Success.
 *      - Others: 	Fail.
 */
stm_err_t hd44780_shift_cursor_backward(hd44780_handle_t handle, uint8_t step);

/*
 * @brief   Destroy LCD handle structure.
 * @param   handle Handle structure.
 * @return	None.
 */
void hd44780_destroy(hd44780_handle_t handle);
 

#ifdef __cplusplus
}
#endif

#endif /* _HD44780_H_ */

