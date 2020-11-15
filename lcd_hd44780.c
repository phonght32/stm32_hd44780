#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "stm_log.h"
#include "include/lcd_hd44780.h"

#define LCD_TICK_DELAY_DEFAULT		50

#define LCD_INIT_ERR_STR		"lcd init error"
#define LCD_READ_ERR_STR		"lcd read error"
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

typedef stm_err_t (*init_func)(lcd_hd44780_hardware_info_t hw_info);
typedef stm_err_t (*write_func)(lcd_hd44780_hardware_info_t hw_info, uint8_t data);
typedef stm_err_t (*read_func)(lcd_hd44780_hardware_info_t hw_info, uint8_t *buf);
typedef void (*wait_func)(lcd_hd44780_handle_t handle);

typedef struct lcd_hd44780 {
	lcd_hd44780_size_t 				size;
	lcd_hd44780_comm_mode_t 		mode;
	lcd_hd44780_hardware_info_t		hw_info;
	write_func 						_write_cmd;
	write_func 						_write_data;
	wait_func 						_wait;
	SemaphoreHandle_t				lock;
} lcd_hd44780_t;


stm_err_t _init_mode_4bit(lcd_hd44780_hardware_info_t hw_info)
{
	gpio_cfg_t gpio_cfg;
	gpio_cfg.mode = GPIO_OUTPUT_PP;
	gpio_cfg.reg_pull_mode = GPIO_REG_PULL_NONE;

	gpio_cfg.gpio_port = hw_info.gpio_port_rs;
	gpio_cfg.gpio_num = hw_info.gpio_num_rs;
	LCD_CHECK(!gpio_config(&gpio_cfg), LCD_INIT_ERR_STR, return STM_FAIL);
	LCD_CHECK(!gpio_set_level(hw_info.gpio_port_rs, hw_info.gpio_num_rs, 0), LCD_INIT_ERR_STR, return STM_FAIL);

	if ((hw_info.gpio_port_rw != -1) && (hw_info.gpio_num_rw != -1)) {
		gpio_cfg.gpio_port = hw_info.gpio_port_rw;
		gpio_cfg.gpio_num = hw_info.gpio_num_rw;
		LCD_CHECK(!gpio_config(&gpio_cfg), LCD_INIT_ERR_STR, return STM_FAIL);
		LCD_CHECK(!gpio_set_level(hw_info.gpio_port_rw, hw_info.gpio_num_rw, 0), LCD_INIT_ERR_STR, return STM_FAIL);
	}

	gpio_cfg.gpio_port = hw_info.gpio_port_en;
	gpio_cfg.gpio_num = hw_info.gpio_num_en;
	LCD_CHECK(!gpio_config(&gpio_cfg), LCD_INIT_ERR_STR, return STM_FAIL);
	LCD_CHECK(!gpio_set_level(hw_info.gpio_port_en, hw_info.gpio_num_en, 0), LCD_INIT_ERR_STR, return STM_FAIL);

	gpio_cfg.gpio_port = hw_info.gpio_port_d4;
	gpio_cfg.gpio_num = hw_info.gpio_num_d4;
	LCD_CHECK(!gpio_config(&gpio_cfg), LCD_INIT_ERR_STR, return STM_FAIL);
	LCD_CHECK(!gpio_set_level(hw_info.gpio_port_d4, hw_info.gpio_num_d4, 0), LCD_INIT_ERR_STR, return STM_FAIL);

	gpio_cfg.gpio_port = hw_info.gpio_port_d5;
	gpio_cfg.gpio_num = hw_info.gpio_num_d5;
	LCD_CHECK(!gpio_config(&gpio_cfg), LCD_INIT_ERR_STR, return STM_FAIL);
	LCD_CHECK(!gpio_set_level(hw_info.gpio_port_d5, hw_info.gpio_num_d5, 0), LCD_INIT_ERR_STR, return STM_FAIL);

	gpio_cfg.gpio_port = hw_info.gpio_port_d6;
	gpio_cfg.gpio_num = hw_info.gpio_num_d6;
	LCD_CHECK(!gpio_config(&gpio_cfg), LCD_INIT_ERR_STR, return STM_FAIL);
	LCD_CHECK(!gpio_set_level(hw_info.gpio_port_d6, hw_info.gpio_num_d6, 0), LCD_INIT_ERR_STR, return STM_FAIL);

	gpio_cfg.gpio_port = hw_info.gpio_port_d7;
	gpio_cfg.gpio_num = hw_info.gpio_num_d7;
	LCD_CHECK(!gpio_config(&gpio_cfg), LCD_INIT_ERR_STR, return STM_FAIL);
	LCD_CHECK(!gpio_set_level(hw_info.gpio_port_d7, hw_info.gpio_num_d7, 0), LCD_INIT_ERR_STR, return STM_FAIL);

	return STM_OK;
}

stm_err_t _write_cmd_4bit(lcd_hd44780_hardware_info_t hw_info, uint8_t cmd)
{
	bool bit_data;
	uint8_t nibble_h = cmd >> 4 & 0x0F;
	uint8_t nibble_l = cmd & 0x0F;

	/* Set hw_info RS to write to command register */
	LCD_CHECK(!gpio_set_level(hw_info.gpio_port_rs, hw_info.gpio_num_rs, false), LCD_WRITE_CMD_ERR_STR, return STM_FAIL);

	if ((hw_info.gpio_port_rw != -1) && (hw_info.gpio_num_rw != -1)) {
		LCD_CHECK(!gpio_set_level(hw_info.gpio_port_rw, hw_info.gpio_num_rw, false), LCD_WRITE_CMD_ERR_STR, return STM_FAIL);
	}

	/* Write high nibble */
	bit_data = (nibble_h >> 0) & 0x01;
	LCD_CHECK(!gpio_set_level(hw_info.gpio_port_d4, hw_info.gpio_num_d4, bit_data), LCD_WRITE_CMD_ERR_STR, return STM_FAIL);
	bit_data = (nibble_h >> 1) & 0x01;
	LCD_CHECK(!gpio_set_level(hw_info.gpio_port_d5, hw_info.gpio_num_d5, bit_data), LCD_WRITE_CMD_ERR_STR, return STM_FAIL);
	bit_data = (nibble_h >> 2) & 0x01;
	LCD_CHECK(!gpio_set_level(hw_info.gpio_port_d6, hw_info.gpio_num_d6, bit_data), LCD_WRITE_CMD_ERR_STR, return STM_FAIL);
	bit_data = (nibble_h >> 3) & 0x01;
	LCD_CHECK(!gpio_set_level(hw_info.gpio_port_d7, hw_info.gpio_num_d7, bit_data), LCD_WRITE_CMD_ERR_STR, return STM_FAIL);

	LCD_CHECK(!gpio_set_level(hw_info.gpio_port_en, hw_info.gpio_num_en, true), LCD_WRITE_CMD_ERR_STR, return STM_FAIL);
	vTaskDelay(1 / portTICK_PERIOD_MS);
	LCD_CHECK(!gpio_set_level(hw_info.gpio_port_en, hw_info.gpio_num_en, false), LCD_WRITE_CMD_ERR_STR, return STM_FAIL);
	vTaskDelay(1 / portTICK_PERIOD_MS);

	bit_data = (nibble_l >> 0) & 0x01;
	LCD_CHECK(!gpio_set_level(hw_info.gpio_port_d4, hw_info.gpio_num_d4, bit_data), LCD_WRITE_CMD_ERR_STR, return STM_FAIL);
	bit_data = (nibble_l >> 1) & 0x01;
	LCD_CHECK(!gpio_set_level(hw_info.gpio_port_d5, hw_info.gpio_num_d5, bit_data), LCD_WRITE_CMD_ERR_STR, return STM_FAIL);
	bit_data = (nibble_l >> 2) & 0x01;
	LCD_CHECK(!gpio_set_level(hw_info.gpio_port_d6, hw_info.gpio_num_d6, bit_data), LCD_WRITE_CMD_ERR_STR, return STM_FAIL);
	bit_data = (nibble_l >> 3) & 0x01;
	LCD_CHECK(!gpio_set_level(hw_info.gpio_port_d7, hw_info.gpio_num_d7, bit_data), LCD_WRITE_CMD_ERR_STR, return STM_FAIL);

	LCD_CHECK(!gpio_set_level(hw_info.gpio_port_en, hw_info.gpio_num_en, true), LCD_WRITE_CMD_ERR_STR, return STM_FAIL);
	vTaskDelay(1 / portTICK_PERIOD_MS);
	LCD_CHECK(!gpio_set_level(hw_info.gpio_port_en, hw_info.gpio_num_en, false), LCD_WRITE_CMD_ERR_STR, return STM_FAIL);
	vTaskDelay(1 / portTICK_PERIOD_MS);

	return STM_OK;
}

stm_err_t _write_data_4bit(lcd_hd44780_hardware_info_t hw_info, uint8_t data)
{
	bool bit_data;
	uint8_t nibble_h = data >> 4 & 0x0F;
	uint8_t nibble_l = data & 0x0F;

	/* Set hw_info RS to high to write to data register */
	LCD_CHECK(!gpio_set_level(hw_info.gpio_port_rs, hw_info.gpio_num_rs, true), LCD_WRITE_CMD_ERR_STR, return STM_FAIL);

	if ((hw_info.gpio_port_rw != -1) && (hw_info.gpio_num_rw != -1)) {
		LCD_CHECK(!gpio_set_level(hw_info.gpio_port_rw, hw_info.gpio_num_rw, false), LCD_WRITE_CMD_ERR_STR, return STM_FAIL);
	}

	/* Write high nibble */
	bit_data = (nibble_h >> 0) & 0x01;
	LCD_CHECK(!gpio_set_level(hw_info.gpio_port_d4, hw_info.gpio_num_d4, bit_data), LCD_WRITE_CMD_ERR_STR, return STM_FAIL);
	bit_data = (nibble_h >> 1) & 0x01;
	LCD_CHECK(!gpio_set_level(hw_info.gpio_port_d5, hw_info.gpio_num_d5, bit_data), LCD_WRITE_CMD_ERR_STR, return STM_FAIL);
	bit_data = (nibble_h >> 2) & 0x01;
	LCD_CHECK(!gpio_set_level(hw_info.gpio_port_d6, hw_info.gpio_num_d6, bit_data), LCD_WRITE_CMD_ERR_STR, return STM_FAIL);
	bit_data = (nibble_h >> 3) & 0x01;
	LCD_CHECK(!gpio_set_level(hw_info.gpio_port_d7, hw_info.gpio_num_d7, bit_data), LCD_WRITE_CMD_ERR_STR, return STM_FAIL);

	LCD_CHECK(!gpio_set_level(hw_info.gpio_port_en, hw_info.gpio_num_en, true), LCD_WRITE_CMD_ERR_STR, return STM_FAIL);
	vTaskDelay(1 / portTICK_PERIOD_MS);
	LCD_CHECK(!gpio_set_level(hw_info.gpio_port_en, hw_info.gpio_num_en, false), LCD_WRITE_CMD_ERR_STR, return STM_FAIL);
	vTaskDelay(1 / portTICK_PERIOD_MS);

	bit_data = (nibble_l >> 0) & 0x01;
	LCD_CHECK(!gpio_set_level(hw_info.gpio_port_d4, hw_info.gpio_num_d4, bit_data), LCD_WRITE_CMD_ERR_STR, return STM_FAIL);
	bit_data = (nibble_l >> 1) & 0x01;
	LCD_CHECK(!gpio_set_level(hw_info.gpio_port_d5, hw_info.gpio_num_d5, bit_data), LCD_WRITE_CMD_ERR_STR, return STM_FAIL);
	bit_data = (nibble_l >> 2) & 0x01;
	LCD_CHECK(!gpio_set_level(hw_info.gpio_port_d6, hw_info.gpio_num_d6, bit_data), LCD_WRITE_CMD_ERR_STR, return STM_FAIL);
	bit_data = (nibble_l >> 3) & 0x01;
	LCD_CHECK(!gpio_set_level(hw_info.gpio_port_d7, hw_info.gpio_num_d7, bit_data), LCD_WRITE_CMD_ERR_STR, return STM_FAIL);

	LCD_CHECK(!gpio_set_level(hw_info.gpio_port_en, hw_info.gpio_num_en, true), LCD_WRITE_CMD_ERR_STR, return STM_FAIL);
	vTaskDelay(1 / portTICK_PERIOD_MS);
	LCD_CHECK(!gpio_set_level(hw_info.gpio_port_en, hw_info.gpio_num_en, false), LCD_WRITE_CMD_ERR_STR, return STM_FAIL);
	vTaskDelay(1 / portTICK_PERIOD_MS);

	return STM_OK;
}

stm_err_t _read_4bit(lcd_hd44780_hardware_info_t hw_info, uint8_t *buf)
{
	gpio_cfg_t gpio_cfg;
	bool bit_data;
	uint8_t nibble_h, nibble_l;

	/* Set GPIOs as input mode */
	gpio_cfg.mode = GPIO_INPUT;
	gpio_cfg.reg_pull_mode = GPIO_REG_PULL_NONE;

	gpio_cfg.gpio_port = hw_info.gpio_port_d4;
	gpio_cfg.gpio_num = hw_info.gpio_num_d4;
	LCD_CHECK(!gpio_config(&gpio_cfg), LCD_INIT_ERR_STR, return STM_FAIL);

	gpio_cfg.gpio_port = hw_info.gpio_port_d5;
	gpio_cfg.gpio_num = hw_info.gpio_num_d5;
	LCD_CHECK(!gpio_config(&gpio_cfg), LCD_INIT_ERR_STR, return STM_FAIL);

	gpio_cfg.gpio_port = hw_info.gpio_port_d6;
	gpio_cfg.gpio_num = hw_info.gpio_num_d6;
	LCD_CHECK(!gpio_config(&gpio_cfg), LCD_INIT_ERR_STR, return STM_FAIL);

	gpio_cfg.gpio_port = hw_info.gpio_port_d7;
	gpio_cfg.gpio_num = hw_info.gpio_num_d7;
	LCD_CHECK(!gpio_config(&gpio_cfg), LCD_INIT_ERR_STR, return STM_FAIL);

	/* Read high nibble */
	LCD_CHECK(!gpio_set_level(hw_info.gpio_port_en, hw_info.gpio_num_en, true), LCD_READ_ERR_STR, return STM_FAIL);
	vTaskDelay(1 / portTICK_PERIOD_MS);
	bit_data = gpio_get_level(hw_info.gpio_port_d4, hw_info.gpio_num_d4);
	if (bit_data)
		nibble_h |= (1 << 0);
	bit_data = gpio_get_level(hw_info.gpio_port_d5, hw_info.gpio_num_d5);
	if (bit_data)
		nibble_h |= (1 << 1);
	bit_data = gpio_get_level(hw_info.gpio_port_d6, hw_info.gpio_num_d6);
	if (bit_data)
		nibble_h |= (1 << 2);
	bit_data = gpio_get_level(hw_info.gpio_port_d7, hw_info.gpio_num_d7);
	if (bit_data)
		nibble_h |= (1 << 3);
	LCD_CHECK(!gpio_set_level(hw_info.gpio_port_en, hw_info.gpio_num_en, false), LCD_READ_ERR_STR, return STM_FAIL);
	vTaskDelay(1 / portTICK_PERIOD_MS);

	/* Read low nibble */
	LCD_CHECK(!gpio_set_level(hw_info.gpio_port_en, hw_info.gpio_num_en, true), LCD_READ_ERR_STR, return STM_FAIL);
	vTaskDelay(1 / portTICK_PERIOD_MS);
	bit_data = gpio_get_level(hw_info.gpio_port_d4, hw_info.gpio_num_d4);
	if (bit_data)
		nibble_l |= (1 << 0);
	bit_data = gpio_get_level(hw_info.gpio_port_d5, hw_info.gpio_num_d5);
	if (bit_data)
		nibble_l |= (1 << 1);
	bit_data = gpio_get_level(hw_info.gpio_port_d6, hw_info.gpio_num_d6);
	if (bit_data)
		nibble_l |= (1 << 2);
	bit_data = gpio_get_level(hw_info.gpio_port_d7, hw_info.gpio_num_d7);
	if (bit_data)
		nibble_l |= (1 << 3);
	LCD_CHECK(!gpio_set_level(hw_info.gpio_port_en, hw_info.gpio_num_en, false), LCD_READ_ERR_STR, return STM_FAIL);
	vTaskDelay(1 / portTICK_PERIOD_MS);

	/* Set GPIOs as output mode */
	gpio_cfg.mode = GPIO_OUTPUT_PP;
	gpio_cfg.reg_pull_mode = GPIO_REG_PULL_NONE;

	gpio_cfg.gpio_port = hw_info.gpio_port_d4;
	gpio_cfg.gpio_num = hw_info.gpio_num_d4;
	LCD_CHECK(!gpio_config(&gpio_cfg), LCD_INIT_ERR_STR, return STM_FAIL);

	gpio_cfg.gpio_port = hw_info.gpio_port_d5;
	gpio_cfg.gpio_num = hw_info.gpio_num_d5;
	LCD_CHECK(!gpio_config(&gpio_cfg), LCD_INIT_ERR_STR, return STM_FAIL);

	gpio_cfg.gpio_port = hw_info.gpio_port_d6;
	gpio_cfg.gpio_num = hw_info.gpio_num_d6;
	LCD_CHECK(!gpio_config(&gpio_cfg), LCD_INIT_ERR_STR, return STM_FAIL);

	gpio_cfg.gpio_port = hw_info.gpio_port_d7;
	gpio_cfg.gpio_num = hw_info.gpio_num_d7;
	LCD_CHECK(!gpio_config(&gpio_cfg), LCD_INIT_ERR_STR, return STM_FAIL);

	/* Convert data */
	*buf = ((nibble_h << 4) | nibble_l);

	return STM_OK;
}

static void _wait_with_delay(lcd_hd44780_handle_t handle)
{
	vTaskDelay(2 / portTICK_PERIOD_MS);
}

static void _wait_with_pinrw(lcd_hd44780_handle_t handle)
{
	read_func _read;
	uint8_t temp_val;

	if (handle->mode == LCD_HD44780_COMM_MODE_4BIT) {
		_read = _read_4bit;
	}
	
	while (1) {
		gpio_set_level(handle->hw_info.gpio_port_rs, handle->hw_info.gpio_num_rs, false);
		gpio_set_level(handle->hw_info.gpio_port_rw, handle->hw_info.gpio_num_rw, true);

		_read(handle->hw_info, &temp_val);
		if ((temp_val & 0x80) == 0) 
			break;
	}
}

static init_func _get_init_func(lcd_hd44780_comm_mode_t mode)
{
	if (mode == LCD_HD44780_COMM_MODE_4BIT) {
		return _init_mode_4bit;
	}
}

static write_func _get_write_cmd_func(lcd_hd44780_comm_mode_t mode)
{
	if (mode == LCD_HD44780_COMM_MODE_4BIT) {
		return _write_cmd_4bit;
	}
}

static write_func _get_write_data_func(lcd_hd44780_comm_mode_t mode)
{
	if (mode == LCD_HD44780_COMM_MODE_4BIT) {
		return _write_data_4bit;
	}
}

static wait_func _get_wait_func(lcd_hd44780_hardware_info_t hw_info)
{
	if ((hw_info.gpio_port_rw == -1) && (hw_info.gpio_num_rw == -1)) {
		return _wait_with_delay;
	} else {
		return _wait_with_pinrw;
	}
}

void _lcd_hd44780_cleanup(lcd_hd44780_handle_t handle)
{
	free(handle);
}

lcd_hd44780_handle_t lcd_hd44780_init(lcd_hd44780_cfg_t *config)
{
	/* Allocate memory for handle structure */
	lcd_hd44780_handle_t handle = calloc(1, sizeof(lcd_hd44780_t));
	LCD_CHECK(handle, LCD_INIT_ERR_STR, return NULL);

	init_func _init_func = _get_init_func(config->mode);
	write_func _write_cmd = _get_write_cmd_func(config->mode);

	/* Make sure that RS pin not used in serial mode */
	if (config->mode == LCD_HD44780_COMM_MODE_SERIAL) {
		config->hw_info.gpio_port_rw = -1;
		config->hw_info.gpio_num_rw = -1;
	}

	/* Configure hw_infos */
	LCD_CHECK(!_init_func(config->hw_info), LCD_INIT_ERR_STR, {_lcd_hd44780_cleanup(handle); return NULL;});

	LCD_CHECK(!_write_cmd(config->hw_info, 0x02), LCD_INIT_ERR_STR, {_lcd_hd44780_cleanup(handle); return NULL;});
	vTaskDelay(LCD_TICK_DELAY_DEFAULT / portTICK_PERIOD_MS);

	LCD_CHECK(!_write_cmd(config->hw_info, 0x28), LCD_INIT_ERR_STR, {_lcd_hd44780_cleanup(handle); return NULL;});
	vTaskDelay(LCD_TICK_DELAY_DEFAULT / portTICK_PERIOD_MS);

	LCD_CHECK(!_write_cmd(config->hw_info, 0x06), LCD_INIT_ERR_STR, {_lcd_hd44780_cleanup(handle); return NULL;});
	vTaskDelay(LCD_TICK_DELAY_DEFAULT / portTICK_PERIOD_MS);

	LCD_CHECK(!_write_cmd(config->hw_info, 0x0C), LCD_INIT_ERR_STR, {_lcd_hd44780_cleanup(handle); return NULL;});
	vTaskDelay(LCD_TICK_DELAY_DEFAULT / portTICK_PERIOD_MS);

	LCD_CHECK(!_write_cmd(config->hw_info, 0x01), LCD_INIT_ERR_STR, {_lcd_hd44780_cleanup(handle); return NULL;});
	vTaskDelay(LCD_TICK_DELAY_DEFAULT / portTICK_PERIOD_MS);

	/* Update handle structure */
	handle->size = config->size;
	handle->mode = config->mode;
	handle->hw_info = config->hw_info;
	handle->_write_cmd = _get_write_cmd_func(config->mode);
	handle->_write_data = _get_write_data_func(config->mode);
	handle->_wait = _get_wait_func(config->hw_info);
	handle->lock = mutex_create();

	return handle;
}

stm_err_t lcd_hd44780_clear(lcd_hd44780_handle_t handle)
{
	mutex_lock(handle->lock);
	handle->_write_cmd(handle->hw_info, 0x01);
	handle->_wait(handle);
	mutex_unlock(handle->lock);

	return STM_OK;
}

stm_err_t lcd_hd44780_home(lcd_hd44780_handle_t handle)
{
	mutex_lock(handle->lock);
	handle->_write_cmd(handle->hw_info, 0x02);
	handle->_wait(handle);
	mutex_unlock(handle->lock);

	return STM_OK;
}

stm_err_t lcd_hd44780_write_string(lcd_hd44780_handle_t handle, uint8_t *str)
{
	mutex_lock(handle->lock);
	while (*str) {
		handle->_write_data(handle->hw_info, *str);
		str++;
	}
	mutex_unlock(handle->lock);

	return STM_OK;
}