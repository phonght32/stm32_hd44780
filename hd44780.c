#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "stm_log.h"
#include "include/hd44780.h"

#define TICK_DELAY_DEFAULT		100
#define I2C_ADDR				(0x27<<1)

#define INIT_ERR_STR				"lcd init error"
#define READ_ERR_STR				"lcd read error"
#define HOME_ERR_STR				"lcd home error"
#define WRITE_CMD_ERR_STR			"lcd write command error"
#define WRITE_DATA_ERR_STR			"lcd write data error"
#define WRITE_STR_ERR_STR			"lcd write string error"
#define WRITE_CHR_ERR_STR			"lcd write char error"
#define WRITE_INT_ERR_STR			"lcd write integer error"
#define WRITE_FLOAT_ERR_STR			"lcd write float error"
#define CLEAR_ERR_STR				"lcd clear error"
#define HOME_ERR_STR				"lcd home error"
#define GOTOXY_ERR_STR				"lcd goto position (x,y) error"
#define SHIFT_CURSOR_ERR_STR		"lcd shift cursor error"

#define mutex_lock(x)			while (xSemaphoreTake(x, portMAX_DELAY) != pdPASS)
#define mutex_unlock(x) 		xSemaphoreGive(x)
#define mutex_create()			xSemaphoreCreateMutex()
#define mutex_destroy(x) 		vQueueDelete(x)

static const char *TAG = "HD44780";

#define HD44780_CHECK(a, str, action) if(!(a)) {								\
	STM_LOGE(TAG, "%s:%d (%s):%s", __FILE__, __LINE__, __FUNCTION__, str);	\
	action;																	\
}

typedef stm_err_t (*init_func)(hd44780_hw_info_t hw_info);
typedef stm_err_t (*write_func)(hd44780_hw_info_t hw_info, uint8_t data);
typedef stm_err_t (*read_func)(hd44780_hw_info_t hw_info, uint8_t *buf);
typedef void (*wait_func)(hd44780_handle_t handle);

typedef struct hd44780 {
	hd44780_size_t 				size;
	hd44780_comm_mode_t 		mode;
	hd44780_hw_info_t		hw_info;
	write_func 						_write_cmd;
	write_func 						_write_data;
	wait_func 						_wait;
	SemaphoreHandle_t				lock;
} hd44780_t;


stm_err_t _init_mode_4bit(hd44780_hw_info_t hw_info)
{
	gpio_cfg_t gpio_cfg;
	gpio_cfg.mode = GPIO_OUTPUT_PP;
	gpio_cfg.reg_pull_mode = GPIO_REG_PULL_NONE;

	gpio_cfg.gpio_port = hw_info.gpio_port_rs;
	gpio_cfg.gpio_num = hw_info.gpio_num_rs;
	HD44780_CHECK(!gpio_config(&gpio_cfg), INIT_ERR_STR, return STM_FAIL);
	HD44780_CHECK(!gpio_set_level(hw_info.gpio_port_rs, hw_info.gpio_num_rs, 0), INIT_ERR_STR, return STM_FAIL);

	if ((hw_info.gpio_port_rw != -1) && (hw_info.gpio_num_rw != -1)) {
		gpio_cfg.gpio_port = hw_info.gpio_port_rw;
		gpio_cfg.gpio_num = hw_info.gpio_num_rw;
		HD44780_CHECK(!gpio_config(&gpio_cfg), INIT_ERR_STR, return STM_FAIL);
		HD44780_CHECK(!gpio_set_level(hw_info.gpio_port_rw, hw_info.gpio_num_rw, 0), INIT_ERR_STR, return STM_FAIL);
	}

	gpio_cfg.gpio_port = hw_info.gpio_port_en;
	gpio_cfg.gpio_num = hw_info.gpio_num_en;
	HD44780_CHECK(!gpio_config(&gpio_cfg), INIT_ERR_STR, return STM_FAIL);
	HD44780_CHECK(!gpio_set_level(hw_info.gpio_port_en, hw_info.gpio_num_en, 0), INIT_ERR_STR, return STM_FAIL);

	gpio_cfg.gpio_port = hw_info.gpio_port_d4;
	gpio_cfg.gpio_num = hw_info.gpio_num_d4;
	HD44780_CHECK(!gpio_config(&gpio_cfg), INIT_ERR_STR, return STM_FAIL);
	HD44780_CHECK(!gpio_set_level(hw_info.gpio_port_d4, hw_info.gpio_num_d4, 0), INIT_ERR_STR, return STM_FAIL);

	gpio_cfg.gpio_port = hw_info.gpio_port_d5;
	gpio_cfg.gpio_num = hw_info.gpio_num_d5;
	HD44780_CHECK(!gpio_config(&gpio_cfg), INIT_ERR_STR, return STM_FAIL);
	HD44780_CHECK(!gpio_set_level(hw_info.gpio_port_d5, hw_info.gpio_num_d5, 0), INIT_ERR_STR, return STM_FAIL);

	gpio_cfg.gpio_port = hw_info.gpio_port_d6;
	gpio_cfg.gpio_num = hw_info.gpio_num_d6;
	HD44780_CHECK(!gpio_config(&gpio_cfg), INIT_ERR_STR, return STM_FAIL);
	HD44780_CHECK(!gpio_set_level(hw_info.gpio_port_d6, hw_info.gpio_num_d6, 0), INIT_ERR_STR, return STM_FAIL);

	gpio_cfg.gpio_port = hw_info.gpio_port_d7;
	gpio_cfg.gpio_num = hw_info.gpio_num_d7;
	HD44780_CHECK(!gpio_config(&gpio_cfg), INIT_ERR_STR, return STM_FAIL);
	HD44780_CHECK(!gpio_set_level(hw_info.gpio_port_d7, hw_info.gpio_num_d7, 0), INIT_ERR_STR, return STM_FAIL);

	return STM_OK;
}

stm_err_t _init_mode_8bit(hd44780_hw_info_t hw_info)
{
	return STM_OK;
}

stm_err_t _init_mode_serial(hd44780_hw_info_t hw_info)
{
	return STM_OK;
}

stm_err_t _write_cmd_4bit(hd44780_hw_info_t hw_info, uint8_t cmd)
{
	bool bit_data;
	uint8_t nibble_h = cmd >> 4 & 0x0F;
	uint8_t nibble_l = cmd & 0x0F;

	/* Set hw_info RS to write to command register */
	HD44780_CHECK(!gpio_set_level(hw_info.gpio_port_rs, hw_info.gpio_num_rs, false), WRITE_CMD_ERR_STR, return STM_FAIL);

	if ((hw_info.gpio_port_rw != -1) && (hw_info.gpio_num_rw != -1)) {
		HD44780_CHECK(!gpio_set_level(hw_info.gpio_port_rw, hw_info.gpio_num_rw, false), WRITE_CMD_ERR_STR, return STM_FAIL);
	}

	/* Write high nibble */
	bit_data = (nibble_h >> 0) & 0x01;
	HD44780_CHECK(!gpio_set_level(hw_info.gpio_port_d4, hw_info.gpio_num_d4, bit_data), WRITE_CMD_ERR_STR, return STM_FAIL);
	bit_data = (nibble_h >> 1) & 0x01;
	HD44780_CHECK(!gpio_set_level(hw_info.gpio_port_d5, hw_info.gpio_num_d5, bit_data), WRITE_CMD_ERR_STR, return STM_FAIL);
	bit_data = (nibble_h >> 2) & 0x01;
	HD44780_CHECK(!gpio_set_level(hw_info.gpio_port_d6, hw_info.gpio_num_d6, bit_data), WRITE_CMD_ERR_STR, return STM_FAIL);
	bit_data = (nibble_h >> 3) & 0x01;
	HD44780_CHECK(!gpio_set_level(hw_info.gpio_port_d7, hw_info.gpio_num_d7, bit_data), WRITE_CMD_ERR_STR, return STM_FAIL);

	HD44780_CHECK(!gpio_set_level(hw_info.gpio_port_en, hw_info.gpio_num_en, true), WRITE_CMD_ERR_STR, return STM_FAIL);
	vTaskDelay(1 / portTICK_PERIOD_MS);
	HD44780_CHECK(!gpio_set_level(hw_info.gpio_port_en, hw_info.gpio_num_en, false), WRITE_CMD_ERR_STR, return STM_FAIL);
	vTaskDelay(1 / portTICK_PERIOD_MS);

	bit_data = (nibble_l >> 0) & 0x01;
	HD44780_CHECK(!gpio_set_level(hw_info.gpio_port_d4, hw_info.gpio_num_d4, bit_data), WRITE_CMD_ERR_STR, return STM_FAIL);
	bit_data = (nibble_l >> 1) & 0x01;
	HD44780_CHECK(!gpio_set_level(hw_info.gpio_port_d5, hw_info.gpio_num_d5, bit_data), WRITE_CMD_ERR_STR, return STM_FAIL);
	bit_data = (nibble_l >> 2) & 0x01;
	HD44780_CHECK(!gpio_set_level(hw_info.gpio_port_d6, hw_info.gpio_num_d6, bit_data), WRITE_CMD_ERR_STR, return STM_FAIL);
	bit_data = (nibble_l >> 3) & 0x01;
	HD44780_CHECK(!gpio_set_level(hw_info.gpio_port_d7, hw_info.gpio_num_d7, bit_data), WRITE_CMD_ERR_STR, return STM_FAIL);

	HD44780_CHECK(!gpio_set_level(hw_info.gpio_port_en, hw_info.gpio_num_en, true), WRITE_CMD_ERR_STR, return STM_FAIL);
	vTaskDelay(1 / portTICK_PERIOD_MS);
	HD44780_CHECK(!gpio_set_level(hw_info.gpio_port_en, hw_info.gpio_num_en, false), WRITE_CMD_ERR_STR, return STM_FAIL);
	vTaskDelay(1 / portTICK_PERIOD_MS);

	return STM_OK;
}

stm_err_t _write_cmd_8bit(hd44780_hw_info_t hw_info, uint8_t cmd)
{
	return STM_OK;
}

stm_err_t _write_cmd_serial(hd44780_hw_info_t hw_info, uint8_t cmd)
{
	uint8_t buf_send[4];
	buf_send[0] = (cmd & 0xF0) | 0x04;
	buf_send[1] = (cmd & 0xF0);
	buf_send[2] = ((cmd << 4) & 0xF0) | 0x04;
	buf_send[3] = ((cmd << 4) & 0xF0) | 0x08;

	HD44780_CHECK(!i2c_master_write_bytes(hw_info.i2c_num, I2C_ADDR, buf_send, 4, TICK_DELAY_DEFAULT), WRITE_CMD_ERR_STR, return STM_FAIL);

	return STM_OK;
}

stm_err_t _write_data_4bit(hd44780_hw_info_t hw_info, uint8_t data)
{
	bool bit_data;
	uint8_t nibble_h = data >> 4 & 0x0F;
	uint8_t nibble_l = data & 0x0F;

	/* Set hw_info RS to high to write to data register */
	HD44780_CHECK(!gpio_set_level(hw_info.gpio_port_rs, hw_info.gpio_num_rs, true), WRITE_DATA_ERR_STR, return STM_FAIL);

	if ((hw_info.gpio_port_rw != -1) && (hw_info.gpio_num_rw != -1)) {
		HD44780_CHECK(!gpio_set_level(hw_info.gpio_port_rw, hw_info.gpio_num_rw, false), WRITE_DATA_ERR_STR, return STM_FAIL);
	}

	/* Write high nibble */
	bit_data = (nibble_h >> 0) & 0x01;
	HD44780_CHECK(!gpio_set_level(hw_info.gpio_port_d4, hw_info.gpio_num_d4, bit_data), WRITE_DATA_ERR_STR, return STM_FAIL);
	bit_data = (nibble_h >> 1) & 0x01;
	HD44780_CHECK(!gpio_set_level(hw_info.gpio_port_d5, hw_info.gpio_num_d5, bit_data), WRITE_DATA_ERR_STR, return STM_FAIL);
	bit_data = (nibble_h >> 2) & 0x01;
	HD44780_CHECK(!gpio_set_level(hw_info.gpio_port_d6, hw_info.gpio_num_d6, bit_data), WRITE_DATA_ERR_STR, return STM_FAIL);
	bit_data = (nibble_h >> 3) & 0x01;
	HD44780_CHECK(!gpio_set_level(hw_info.gpio_port_d7, hw_info.gpio_num_d7, bit_data), WRITE_DATA_ERR_STR, return STM_FAIL);

	HD44780_CHECK(!gpio_set_level(hw_info.gpio_port_en, hw_info.gpio_num_en, true), WRITE_DATA_ERR_STR, return STM_FAIL);
	vTaskDelay(1 / portTICK_PERIOD_MS);
	HD44780_CHECK(!gpio_set_level(hw_info.gpio_port_en, hw_info.gpio_num_en, false), WRITE_DATA_ERR_STR, return STM_FAIL);
	vTaskDelay(1 / portTICK_PERIOD_MS);

	bit_data = (nibble_l >> 0) & 0x01;
	HD44780_CHECK(!gpio_set_level(hw_info.gpio_port_d4, hw_info.gpio_num_d4, bit_data), WRITE_DATA_ERR_STR, return STM_FAIL);
	bit_data = (nibble_l >> 1) & 0x01;
	HD44780_CHECK(!gpio_set_level(hw_info.gpio_port_d5, hw_info.gpio_num_d5, bit_data), WRITE_DATA_ERR_STR, return STM_FAIL);
	bit_data = (nibble_l >> 2) & 0x01;
	HD44780_CHECK(!gpio_set_level(hw_info.gpio_port_d6, hw_info.gpio_num_d6, bit_data), WRITE_DATA_ERR_STR, return STM_FAIL);
	bit_data = (nibble_l >> 3) & 0x01;
	HD44780_CHECK(!gpio_set_level(hw_info.gpio_port_d7, hw_info.gpio_num_d7, bit_data), WRITE_DATA_ERR_STR, return STM_FAIL);

	HD44780_CHECK(!gpio_set_level(hw_info.gpio_port_en, hw_info.gpio_num_en, true), WRITE_DATA_ERR_STR, return STM_FAIL);
	vTaskDelay(1 / portTICK_PERIOD_MS);
	HD44780_CHECK(!gpio_set_level(hw_info.gpio_port_en, hw_info.gpio_num_en, false), WRITE_DATA_ERR_STR, return STM_FAIL);
	vTaskDelay(1 / portTICK_PERIOD_MS);

	return STM_OK;
}

stm_err_t _write_data_8bit(hd44780_hw_info_t hw_info, uint8_t data)
{
	return STM_OK;
}

stm_err_t _write_data_serial(hd44780_hw_info_t hw_info, uint8_t data)
{
	uint8_t buf_send[4];
	buf_send[0] = (data & 0xF0) | 0x0D;
	buf_send[1] = (data & 0xF0) | 0x09;
	buf_send[2] = ((data << 4) & 0xF0) | 0x0D;
	buf_send[3] = ((data << 4) & 0xF0) | 0x09;

	HD44780_CHECK(!i2c_master_write_bytes(hw_info.i2c_num, I2C_ADDR, buf_send, 4, TICK_DELAY_DEFAULT), WRITE_DATA_ERR_STR, return STM_FAIL);

	return STM_OK;
}

stm_err_t _read_4bit(hd44780_hw_info_t hw_info, uint8_t *buf)
{
	gpio_cfg_t gpio_cfg;
	bool bit_data;
	uint8_t nibble_h = 0, nibble_l = 0;

	/* Set GPIOs as input mode */
	gpio_cfg.mode = GPIO_INPUT;
	gpio_cfg.reg_pull_mode = GPIO_REG_PULL_NONE;

	gpio_cfg.gpio_port = hw_info.gpio_port_d4;
	gpio_cfg.gpio_num = hw_info.gpio_num_d4;
	HD44780_CHECK(!gpio_config(&gpio_cfg), INIT_ERR_STR, return STM_FAIL);

	gpio_cfg.gpio_port = hw_info.gpio_port_d5;
	gpio_cfg.gpio_num = hw_info.gpio_num_d5;
	HD44780_CHECK(!gpio_config(&gpio_cfg), INIT_ERR_STR, return STM_FAIL);

	gpio_cfg.gpio_port = hw_info.gpio_port_d6;
	gpio_cfg.gpio_num = hw_info.gpio_num_d6;
	HD44780_CHECK(!gpio_config(&gpio_cfg), INIT_ERR_STR, return STM_FAIL);

	gpio_cfg.gpio_port = hw_info.gpio_port_d7;
	gpio_cfg.gpio_num = hw_info.gpio_num_d7;
	HD44780_CHECK(!gpio_config(&gpio_cfg), INIT_ERR_STR, return STM_FAIL);

	/* Read high nibble */
	HD44780_CHECK(!gpio_set_level(hw_info.gpio_port_en, hw_info.gpio_num_en, true), READ_ERR_STR, return STM_FAIL);
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
	HD44780_CHECK(!gpio_set_level(hw_info.gpio_port_en, hw_info.gpio_num_en, false), READ_ERR_STR, return STM_FAIL);
	vTaskDelay(1 / portTICK_PERIOD_MS);

	/* Read low nibble */
	HD44780_CHECK(!gpio_set_level(hw_info.gpio_port_en, hw_info.gpio_num_en, true), READ_ERR_STR, return STM_FAIL);
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
	HD44780_CHECK(!gpio_set_level(hw_info.gpio_port_en, hw_info.gpio_num_en, false), READ_ERR_STR, return STM_FAIL);
	vTaskDelay(1 / portTICK_PERIOD_MS);

	/* Set GPIOs as output mode */
	gpio_cfg.mode = GPIO_OUTPUT_PP;
	gpio_cfg.reg_pull_mode = GPIO_REG_PULL_NONE;

	gpio_cfg.gpio_port = hw_info.gpio_port_d4;
	gpio_cfg.gpio_num = hw_info.gpio_num_d4;
	HD44780_CHECK(!gpio_config(&gpio_cfg), INIT_ERR_STR, return STM_FAIL);

	gpio_cfg.gpio_port = hw_info.gpio_port_d5;
	gpio_cfg.gpio_num = hw_info.gpio_num_d5;
	HD44780_CHECK(!gpio_config(&gpio_cfg), INIT_ERR_STR, return STM_FAIL);

	gpio_cfg.gpio_port = hw_info.gpio_port_d6;
	gpio_cfg.gpio_num = hw_info.gpio_num_d6;
	HD44780_CHECK(!gpio_config(&gpio_cfg), INIT_ERR_STR, return STM_FAIL);

	gpio_cfg.gpio_port = hw_info.gpio_port_d7;
	gpio_cfg.gpio_num = hw_info.gpio_num_d7;
	HD44780_CHECK(!gpio_config(&gpio_cfg), INIT_ERR_STR, return STM_FAIL);

	/* Convert data */
	*buf = ((nibble_h << 4) | nibble_l);

	return STM_OK;
}

stm_err_t _read_8bit(hd44780_hw_info_t hw_info, uint8_t *buf)
{
	return STM_OK;
}

static void _wait_with_delay(hd44780_handle_t handle)
{
	vTaskDelay(2 / portTICK_PERIOD_MS);
}

static void _wait_with_pinrw(hd44780_handle_t handle)
{
	read_func _read;
	uint8_t temp_val;

	if (handle->mode == HD44780_COMM_MODE_4BIT) {
		_read = _read_4bit;
	} else if (handle->mode == HD44780_COMM_MODE_8BIT) {
		_read = _read_8bit;
	} else {
		_read = NULL;
	}

	while (1) {
		gpio_set_level(handle->hw_info.gpio_port_rs, handle->hw_info.gpio_num_rs, false);
		gpio_set_level(handle->hw_info.gpio_port_rw, handle->hw_info.gpio_num_rw, true);

		_read(handle->hw_info, &temp_val);
		if ((temp_val & 0x80) == 0)
			break;
	}
}

static init_func _get_init_func(hd44780_comm_mode_t mode)
{
	if (mode == HD44780_COMM_MODE_4BIT) {
		return _init_mode_4bit;
	} else if (mode == HD44780_COMM_MODE_8BIT) {
		return _init_mode_8bit;
	} else {
		return _init_mode_serial;
	}

	return NULL;
}

static write_func _get_write_cmd_func(hd44780_comm_mode_t mode)
{
	if (mode == HD44780_COMM_MODE_4BIT) {
		return _write_cmd_4bit;
	} else if (mode == HD44780_COMM_MODE_8BIT) {
		return _write_cmd_8bit;
	} else {
		return _write_cmd_serial;
	}

	return NULL;
}

static write_func _get_write_data_func(hd44780_comm_mode_t mode)
{
	if (mode == HD44780_COMM_MODE_4BIT) {
		return _write_data_4bit;
	} else if (mode == HD44780_COMM_MODE_8BIT) {
		return _write_data_8bit;
	} else {
		return _write_data_serial;
	}

	return NULL;
}

static wait_func _get_wait_func(hd44780_hw_info_t hw_info)
{
	if ((hw_info.gpio_port_rw == -1) && (hw_info.gpio_num_rw == -1)) {
		return _wait_with_delay;
	} else {
		return _wait_with_pinrw;
	}

	return NULL;
}

void _hd44780_cleanup(hd44780_handle_t handle)
{
	free(handle);
}

hd44780_handle_t hd44780_init(hd44780_cfg_t *config)
{
	/* Check input condition */
	HD44780_CHECK(config, INIT_ERR_STR, return NULL);
	HD44780_CHECK(config->size < HD44780_SIZE_MAX, INIT_ERR_STR, return NULL);
	HD44780_CHECK(config->mode < HD44780_COMM_MODE_MAX, INIT_ERR_STR, return NULL);

	/* Allocate memory for handle structure */
	hd44780_handle_t handle = calloc(1, sizeof(hd44780_t));
	HD44780_CHECK(handle, INIT_ERR_STR, return NULL);

	init_func _init_func = _get_init_func(config->mode);
	write_func _write_cmd = _get_write_cmd_func(config->mode);

	/* Make sure that RS pin not used in serial mode */
	if (config->mode == HD44780_COMM_MODE_SERIAL) {
		config->hw_info.gpio_port_rw = -1;
		config->hw_info.gpio_num_rw = -1;
	}

	/* Configure hw_infos */
	HD44780_CHECK(!_init_func(config->hw_info), INIT_ERR_STR, {_hd44780_cleanup(handle); return NULL;});

	HD44780_CHECK(!_write_cmd(config->hw_info, 0x02), INIT_ERR_STR, {_hd44780_cleanup(handle); return NULL;});
	vTaskDelay(TICK_DELAY_DEFAULT / portTICK_PERIOD_MS);

	HD44780_CHECK(!_write_cmd(config->hw_info, 0x28), INIT_ERR_STR, {_hd44780_cleanup(handle); return NULL;});
	vTaskDelay(TICK_DELAY_DEFAULT / portTICK_PERIOD_MS);

	HD44780_CHECK(!_write_cmd(config->hw_info, 0x06), INIT_ERR_STR, {_hd44780_cleanup(handle); return NULL;});
	vTaskDelay(TICK_DELAY_DEFAULT / portTICK_PERIOD_MS);

	HD44780_CHECK(!_write_cmd(config->hw_info, 0x0C), INIT_ERR_STR, {_hd44780_cleanup(handle); return NULL;});
	vTaskDelay(TICK_DELAY_DEFAULT / portTICK_PERIOD_MS);

	HD44780_CHECK(!_write_cmd(config->hw_info, 0x01), INIT_ERR_STR, {_hd44780_cleanup(handle); return NULL;});
	vTaskDelay(TICK_DELAY_DEFAULT / portTICK_PERIOD_MS);

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

stm_err_t hd44780_clear(hd44780_handle_t handle)
{
	/* Check input condition */
	HD44780_CHECK(handle, CLEAR_ERR_STR, return STM_ERR_INVALID_ARG);

	mutex_lock(handle->lock);

	int ret = handle->_write_cmd(handle->hw_info, 0x01);
	if (ret) {
		STM_LOGE(TAG, CLEAR_ERR_STR);
		mutex_unlock(handle->lock);
		return STM_FAIL;
	}

	handle->_wait(handle);

	mutex_unlock(handle->lock);

	return STM_OK;
}

stm_err_t hd44780_home(hd44780_handle_t handle)
{
	/* Check input condition */
	HD44780_CHECK(handle, HOME_ERR_STR, return STM_ERR_INVALID_ARG);

	mutex_lock(handle->lock);

	int ret = handle->_write_cmd(handle->hw_info, 0x02);
	if (ret) {
		STM_LOGE(TAG, HOME_ERR_STR);
		mutex_unlock(handle->lock);
		return STM_FAIL;
	}

	handle->_wait(handle);

	mutex_unlock(handle->lock);

	return STM_OK;
}

stm_err_t hd44780_write_char(hd44780_handle_t handle, uint8_t chr)
{
	/* Check input condition */
	HD44780_CHECK(handle, WRITE_CHR_ERR_STR, return STM_ERR_INVALID_ARG);

	mutex_lock(handle->lock);

	int ret = handle->_write_data(handle->hw_info, chr);
	if (ret) {
		STM_LOGE(TAG, WRITE_CHR_ERR_STR);
		mutex_unlock(handle->lock);
		return STM_FAIL;
	}

	mutex_unlock(handle->lock);

	return STM_OK;
}

stm_err_t hd44780_write_string(hd44780_handle_t handle, uint8_t *str)
{
	/* Check input condition */
	HD44780_CHECK(handle, WRITE_STR_ERR_STR, return STM_ERR_INVALID_ARG);
	HD44780_CHECK(str, WRITE_STR_ERR_STR, return STM_ERR_INVALID_ARG);

	mutex_lock(handle->lock);
	int ret;

	while (*str) {
		ret = handle->_write_data(handle->hw_info, *str);
		if (ret) {
			STM_LOGE(TAG, WRITE_STR_ERR_STR);
			mutex_unlock(handle->lock);
			return STM_FAIL;
		}
		str++;
	}
	mutex_unlock(handle->lock);

	return STM_OK;
}

stm_err_t hd44780_write_int(hd44780_handle_t handle, int number)
{
	/* Check input condition */
	HD44780_CHECK(handle, WRITE_INT_ERR_STR, return STM_ERR_INVALID_ARG);

	mutex_lock(handle->lock);

	int ret;

	if (number < 0) {
		ret = handle->_write_data(handle->hw_info, '-');
		if (ret) {
			STM_LOGE(TAG, WRITE_INT_ERR_STR);
			mutex_unlock(handle->lock);
			return STM_FAIL;
		}
		number *= -1;
	}

	int num_digit = 1;
	int temp = number;

	while (temp > 9) {
		num_digit++;
		temp /= 10;
	}

	uint8_t buf[num_digit];
	sprintf((char*)buf, "%d", number);

	for (int i = 0; i < num_digit; i++) {
		ret = handle->_write_data(handle->hw_info, buf[i]);
		if (ret) {
			STM_LOGE(TAG, WRITE_INT_ERR_STR);
			mutex_unlock(handle->lock);
			return STM_FAIL;
		}
	}

	mutex_unlock(handle->lock);

	return STM_OK;
}

stm_err_t hd44780_write_float(hd44780_handle_t handle, float number, uint8_t precision)
{
	/* Check input condition */
	HD44780_CHECK(handle, WRITE_INT_ERR_STR, return STM_ERR_INVALID_ARG);

	mutex_lock(handle->lock);

	int ret;

	if (number < 0) {
		ret = handle->_write_data(handle->hw_info, '-');
		if (ret) {
			STM_LOGE(TAG, WRITE_INT_ERR_STR);
			mutex_unlock(handle->lock);
			return STM_FAIL;
		}
		number *= -1;
	}

	int num_digit = 1;
	int temp = (int)number;

	while (temp > 9) {
		num_digit++;
		temp /= 10;
	}

	uint8_t buf[num_digit + 1 + precision];
	uint8_t float_format[7];

	sprintf((char*)float_format, "%%.%df", precision);
	sprintf((char*)buf, (const char*)float_format, number);

	for (int i = 0; i < (num_digit + 1 + precision); i++) {
		ret = handle->_write_data(handle->hw_info, buf[i]);
		if (ret) {
			STM_LOGE(TAG, WRITE_INT_ERR_STR);
			mutex_unlock(handle->lock);
			return STM_FAIL;
		}
	}

	mutex_unlock(handle->lock);

	return STM_OK;
}

stm_err_t hd44780_gotoxy(hd44780_handle_t handle, uint8_t col, uint8_t row)
{
	/* Check input condition */
	HD44780_CHECK(handle, GOTOXY_ERR_STR, return STM_ERR_INVALID_ARG);

	mutex_lock(handle->lock);

	int ret;
	if (row == 0) {
		ret = handle->_write_cmd(handle->hw_info, 0x80 + col);
		if (ret) {
			STM_LOGE(TAG, GOTOXY_ERR_STR);
			mutex_unlock(handle->lock);
			return STM_FAIL;
		}
	}
	else if (row == 1) {
		ret = handle->_write_cmd(handle->hw_info, 0xC0 + col);
		if (ret) {
			STM_LOGE(TAG, GOTOXY_ERR_STR);
			mutex_unlock(handle->lock);
			return STM_FAIL;
		}
	}
	else if (row == 2) {
		ret = handle->_write_cmd(handle->hw_info, 0x94 + col);
		if (ret) {
			STM_LOGE(TAG, GOTOXY_ERR_STR);
			mutex_unlock(handle->lock);
			return STM_FAIL;
		}
	}
	else {
		ret = handle->_write_cmd(handle->hw_info, 0xD4 + col);
		if (ret) {
			STM_LOGE(TAG, GOTOXY_ERR_STR);
			mutex_unlock(handle->lock);
			return STM_FAIL;
		}
	}
	mutex_unlock(handle->lock);

	return STM_OK;
}

stm_err_t hd44780_shift_cursor_forward(hd44780_handle_t handle, uint8_t step)
{
	/* Check input condition */
	HD44780_CHECK(handle, SHIFT_CURSOR_ERR_STR, return STM_ERR_INVALID_ARG);

	mutex_lock(handle->lock);
	int ret;

	for (uint8_t i = 0; i < step; i++) {
		ret = handle->_write_cmd(handle->hw_info, 0x14);
		if (ret) {
			STM_LOGE(TAG, SHIFT_CURSOR_ERR_STR);
			mutex_unlock(handle->lock);
			return STM_FAIL;
		}
	}
	mutex_unlock(handle->lock);

	return STM_OK;
}

stm_err_t hd44780_shift_cursor_backward(hd44780_handle_t handle, uint8_t step)
{
	/* Check input condition */
	HD44780_CHECK(handle, SHIFT_CURSOR_ERR_STR, return STM_ERR_INVALID_ARG);

	mutex_lock(handle->lock);
	int ret;

	for (uint8_t i = 0; i < step; i++) {
		ret = handle->_write_cmd(handle->hw_info, 0x10);
		if (ret) {
			STM_LOGE(TAG, SHIFT_CURSOR_ERR_STR);
			mutex_unlock(handle->lock);
			return STM_FAIL;
		}
	}
	mutex_unlock(handle->lock);

	return STM_OK;
}

void hd44780_destroy(hd44780_handle_t handle)
{
	free(handle);
}