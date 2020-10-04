#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "stm_log.h"
#include "include/lcd_hd44780.h"

#define LCD_TICK_DELAY_DEFAULT		50

#define LCD_INIT_ERR_STR		"lcd init error"
#define LCD_WRITE_CMD_ERR_STR	"lcd write command error"
#define LCD_CLEAR_ERR_STR		"lcd clear error"
#define LCD_HOME_ERR_STR		"lcd home error"

#define mutex_lock(x)			while (xSemaphoreTake(x, portMAX_DELAY) != pdPASS)
#define mutex_unlock(x) 		xSemaphoreGive(x)
#define mutex_create()			xSemaphoreCreateMutex()
#define mutex_destroy(x) 		vQueueDelete(x)

static const char *TAG = "LCD_DRIVER";

#define LCD_CHECK(a, str, action) if(!a) {									\
	STM_LOGE(TAG, "%s:%d (%s):%s", __FILE__, __LINE__, __FUNCTION__, str);	\
	action;																	\
}

typedef stm_err_t (*init_func)(lcd_pin_t pin);
typedef stm_err_t (*write_func)(lcd_pin_t pin, uint8_t data);

typedef struct lcd {
	lcd_size_t 			size;
	lcd_driver_t 		driver;
	lcd_comm_mode_t 	mode;
	lcd_pin_t			pin;
	write_func 			_write_cmd;
	write_func 			_write_data;
	SemaphoreHandle_t	lock;
} lcd_t;


stm_err_t _init_mode_4bit(lcd_pin_t pin) {

	gpio_cfg_t gpio_cfg;
	gpio_cfg.mode = GPIO_OUTPUT_PP;
	gpio_cfg.reg_pull_mode = GPIO_REG_PULL_NONE;

	gpio_cfg.gpio_port = pin.gpio_port_rs;
	gpio_cfg.gpio_num = pin.gpio_num_rs;
	LCD_CHECK(!gpio_config(&gpio_cfg), LCD_INIT_ERR_STR, return STM_FAIL);

	gpio_cfg.gpio_port = pin.gpio_port_rw;
	gpio_cfg.gpio_num = pin.gpio_num_rw;
	LCD_CHECK(!gpio_config(&gpio_cfg), LCD_INIT_ERR_STR, return STM_FAIL);

	gpio_cfg.gpio_port = pin.gpio_port_en;
	gpio_cfg.gpio_num = pin.gpio_num_en;
	LCD_CHECK(!gpio_config(&gpio_cfg), LCD_INIT_ERR_STR, return STM_FAIL);

	gpio_cfg.gpio_port = pin.gpio_port_d4;
	gpio_cfg.gpio_num = pin.gpio_num_d4;
	LCD_CHECK(!gpio_config(&gpio_cfg), LCD_INIT_ERR_STR, return STM_FAIL);

	gpio_cfg.gpio_port = pin.gpio_port_d5;
	gpio_cfg.gpio_num = pin.gpio_num_d5;
	LCD_CHECK(!gpio_config(&gpio_cfg), LCD_INIT_ERR_STR, return STM_FAIL);

	gpio_cfg.gpio_port = pin.gpio_port_d6;
	gpio_cfg.gpio_num = pin.gpio_num_d6;
	LCD_CHECK(!gpio_config(&gpio_cfg), LCD_INIT_ERR_STR, return STM_FAIL);

	gpio_cfg.gpio_port = pin.gpio_port_d7;
	gpio_cfg.gpio_num = pin.gpio_num_d7;
	LCD_CHECK(!gpio_config(&gpio_cfg), LCD_INIT_ERR_STR, return STM_FAIL);

	return STM_OK;
}

stm_err_t _write_cmd_4bit(lcd_pin_t pin, uint8_t cmd) {

	bool bit_data;
	uint8_t nibble_h = cmd >> 4 & 0x0F;
	uint8_t nibble_l = cmd & 0x0F;

	/* Set pin RS to write to command register */
	LCD_CHECK(!gpio_set_level(pin.gpio_port_rs, pin.gpio_num_rs, false), LCD_WRITE_CMD_ERR_STR, return STM_FAIL);

	/* Write high nibble */
	bit_data = (nibble_h >> 0) & 0x01;
	LCD_CHECK(!gpio_set_level(pin.gpio_port_d4, pin.gpio_num_d4, bit_data), LCD_WRITE_CMD_ERR_STR, return STM_FAIL);
	bit_data = (nibble_h >> 1) & 0x01;
	LCD_CHECK(!gpio_set_level(pin.gpio_port_d5, pin.gpio_num_d5, bit_data), LCD_WRITE_CMD_ERR_STR, return STM_FAIL);
	bit_data = (nibble_h >> 2) & 0x01;
	LCD_CHECK(!gpio_set_level(pin.gpio_port_d6, pin.gpio_num_d6, bit_data), LCD_WRITE_CMD_ERR_STR, return STM_FAIL);
	bit_data = (nibble_h >> 3) & 0x01;
	LCD_CHECK(!gpio_set_level(pin.gpio_port_d7, pin.gpio_num_d7, bit_data), LCD_WRITE_CMD_ERR_STR, return STM_FAIL);

	LCD_CHECK(!gpio_set_level(pin.gpio_port_en, pin.gpio_num_en, true), LCD_WRITE_CMD_ERR_STR, return STM_FAIL);
	vTaskDelay(1 / portTICK_PERIOD_MS);
	LCD_CHECK(!gpio_set_level(pin.gpio_port_en, pin.gpio_num_en, false), LCD_WRITE_CMD_ERR_STR, return STM_FAIL);
	vTaskDelay(1 / portTICK_PERIOD_MS);

	bit_data = (nibble_l >> 0) & 0x01;
	LCD_CHECK(!gpio_set_level(pin.gpio_port_d4, pin.gpio_num_d4, bit_data), LCD_WRITE_CMD_ERR_STR, return STM_FAIL);
	bit_data = (nibble_l >> 1) & 0x01;
	LCD_CHECK(!gpio_set_level(pin.gpio_port_d5, pin.gpio_num_d5, bit_data), LCD_WRITE_CMD_ERR_STR, return STM_FAIL);
	bit_data = (nibble_l >> 2) & 0x01;
	LCD_CHECK(!gpio_set_level(pin.gpio_port_d6, pin.gpio_num_d6, bit_data), LCD_WRITE_CMD_ERR_STR, return STM_FAIL);
	bit_data = (nibble_l >> 3) & 0x01;
	LCD_CHECK(!gpio_set_level(pin.gpio_port_d7, pin.gpio_num_d7, bit_data), LCD_WRITE_CMD_ERR_STR, return STM_FAIL);

	LCD_CHECK(!gpio_set_level(pin.gpio_port_en, pin.gpio_num_en, true), LCD_WRITE_CMD_ERR_STR, return STM_FAIL);
	vTaskDelay(1 / portTICK_PERIOD_MS);
	LCD_CHECK(!gpio_set_level(pin.gpio_port_en, pin.gpio_num_en, false), LCD_WRITE_CMD_ERR_STR, return STM_FAIL);
	vTaskDelay(1 / portTICK_PERIOD_MS);

	return STM_OK;
}

stm_err_t _write_data_4bit(lcd_pin_t pin, uint8_t data) {

	bool bit_data;
	uint8_t nibble_h = data >> 4 & 0x0F;
	uint8_t nibble_l = data & 0x0F;

	/* Set pin RS to high to write to data register */
	LCD_CHECK(!gpio_set_level(pin.gpio_port_rs, pin.gpio_num_rs, true), LCD_WRITE_CMD_ERR_STR, return STM_FAIL);

	/* Write high nibble */
	bit_data = (nibble_h >> 0) & 0x01;
	LCD_CHECK(!gpio_set_level(pin.gpio_port_d4, pin.gpio_num_d4, bit_data), LCD_WRITE_CMD_ERR_STR, return STM_FAIL);
	bit_data = (nibble_h >> 1) & 0x01;
	LCD_CHECK(!gpio_set_level(pin.gpio_port_d5, pin.gpio_num_d5, bit_data), LCD_WRITE_CMD_ERR_STR, return STM_FAIL);
	bit_data = (nibble_h >> 2) & 0x01;
	LCD_CHECK(!gpio_set_level(pin.gpio_port_d6, pin.gpio_num_d6, bit_data), LCD_WRITE_CMD_ERR_STR, return STM_FAIL);
	bit_data = (nibble_h >> 3) & 0x01;
	LCD_CHECK(!gpio_set_level(pin.gpio_port_d7, pin.gpio_num_d7, bit_data), LCD_WRITE_CMD_ERR_STR, return STM_FAIL);

	LCD_CHECK(!gpio_set_level(pin.gpio_port_en, pin.gpio_num_en, true), LCD_WRITE_CMD_ERR_STR, return STM_FAIL);
	vTaskDelay(1 / portTICK_PERIOD_MS);
	LCD_CHECK(!gpio_set_level(pin.gpio_port_en, pin.gpio_num_en, false), LCD_WRITE_CMD_ERR_STR, return STM_FAIL);
	vTaskDelay(1 / portTICK_PERIOD_MS);

	bit_data = (nibble_l >> 0) & 0x01;
	LCD_CHECK(!gpio_set_level(pin.gpio_port_d4, pin.gpio_num_d4, bit_data), LCD_WRITE_CMD_ERR_STR, return STM_FAIL);
	bit_data = (nibble_l >> 1) & 0x01;
	LCD_CHECK(!gpio_set_level(pin.gpio_port_d5, pin.gpio_num_d5, bit_data), LCD_WRITE_CMD_ERR_STR, return STM_FAIL);
	bit_data = (nibble_l >> 2) & 0x01;
	LCD_CHECK(!gpio_set_level(pin.gpio_port_d6, pin.gpio_num_d6, bit_data), LCD_WRITE_CMD_ERR_STR, return STM_FAIL);
	bit_data = (nibble_l >> 3) & 0x01;
	LCD_CHECK(!gpio_set_level(pin.gpio_port_d7, pin.gpio_num_d7, bit_data), LCD_WRITE_CMD_ERR_STR, return STM_FAIL);

	LCD_CHECK(!gpio_set_level(pin.gpio_port_en, pin.gpio_num_en, true), LCD_WRITE_CMD_ERR_STR, return STM_FAIL);
	vTaskDelay(1 / portTICK_PERIOD_MS);
	LCD_CHECK(!gpio_set_level(pin.gpio_port_en, pin.gpio_num_en, false), LCD_WRITE_CMD_ERR_STR, return STM_FAIL);
	vTaskDelay(1 / portTICK_PERIOD_MS);

	return STM_OK;
}

void _lcd_cleanup(lcd_handle_t handle) {
	free(handle);
}


lcd_handle_t lcd_init(lcd_cfg_t *config) {
	/* Allocate memory for handle structure */
	lcd_handle_t handle = calloc(1, sizeof(lcd_t));
	LCD_CHECK(handle, LCD_INIT_ERR_STR, return NULL);

	init_func _init_func;
	write_func _write_cmd;
	write_func _write_data;

	switch (config->mode) {
	case LCD_COMM_MODE_4BIT:
		_init_func = _init_mode_4bit;
		_write_cmd = _write_cmd_4bit;
		_write_data = _write_data_4bit;
		break;
	case LCD_COMM_MODE_8BIT:
		break;
	case LCD_COMM_MODE_SERIAL:
		break;
	default:
		break;
	}

	/* Configure pins */
	LCD_CHECK(!_init_func(config->pin), LCD_INIT_ERR_STR, {_lcd_cleanup(handle); return NULL;});

	LCD_CHECK(!_write_cmd(config->pin, 0x02), LCD_INIT_ERR_STR, {_lcd_cleanup(handle); return NULL;});
	vTaskDelay(LCD_TICK_DELAY_DEFAULT / portTICK_PERIOD_MS);

	LCD_CHECK(!_write_cmd(config->pin, 0x28), LCD_INIT_ERR_STR, {_lcd_cleanup(handle); return NULL;});
	vTaskDelay(LCD_TICK_DELAY_DEFAULT / portTICK_PERIOD_MS);

	LCD_CHECK(!_write_cmd(config->pin, 0x06), LCD_INIT_ERR_STR, {_lcd_cleanup(handle); return NULL;});
	vTaskDelay(LCD_TICK_DELAY_DEFAULT / portTICK_PERIOD_MS);

	LCD_CHECK(!_write_cmd(config->pin, 0x0C), LCD_INIT_ERR_STR, {_lcd_cleanup(handle); return NULL;});
	vTaskDelay(LCD_TICK_DELAY_DEFAULT / portTICK_PERIOD_MS);

	LCD_CHECK(!_write_cmd(config->pin, 0x01), LCD_INIT_ERR_STR, {_lcd_cleanup(handle); return NULL;});
	vTaskDelay(LCD_TICK_DELAY_DEFAULT / portTICK_PERIOD_MS);

	/* Update handle structure */
	handle->size = config->size;
	handle->driver = config->driver;
	handle->mode = config->mode;
	handle->pin = config->pin;
	handle->_write_cmd = _write_cmd_4bit;
	handle->_write_data = _write_data_4bit;
	handle->lock = mutex_create();

	return handle;
}

stm_err_t lcd_clear(lcd_handle_t handle) {
	handle->_write_cmd(handle->pin, 0x01);
	vTaskDelay(2 / portTICK_PERIOD_MS);

	return STM_OK;
}

stm_err_t lcd_home(lcd_handle_t handle) {
	handle->_write_cmd(handle->pin, 0x02);
	vTaskDelay(2 / portTICK_PERIOD_MS);

	return STM_OK;
}

stm_err_t lcd_write_string(lcd_handle_t handle, uint8_t *str) {
	while (*str) {
		handle->_write_data(handle->pin, *str);
		str++;
	}

	return STM_OK;
}