#include <i2c_protocol.h>
esp_err_t i2c_master_init()
{
    i2c_config_t config;
    config.master.clk_speed = MASTER_FREQUENTLY;
    config.mode = I2C_MODE_MASTER;
    config.scl_io_num = MASTER_SCL_IO;
    config.sda_io_num = MASTER_SDA_IO;
    config.sda_pullup_en = 1;
    config.scl_pullup_en = 1;
    i2c_param_config(I2C_PORT_NUM,&config);
    return i2c_driver_install(I2C_PORT_NUM,I2C_MODE_MASTER,I2C_MASTER_BUFFER_RX,I2C_MASTER_BUFFER_TX,0);
}
esp_err_t master_write_byte(BYTE SLAY_ADDRESS,uint8_t* data, int data_size)
{
    i2c_cmd_handle_t cmd;
    //step 1: link
    cmd=i2c_cmd_link_create();
    //step 2: start send
    i2c_master_start(cmd);
    //step 3: master_write_byte 
    i2c_master_write_byte(cmd,(SLAY_ADDRESS << 1) | I2C_MASTER_WRITE, ACK_EN);// write
    //step 4: master write char 
    i2c_master_write(cmd, data, data_size,ACK_EN);
    //step 5: stop i2c link
    i2c_master_stop(cmd);
    //step 6 send 
    esp_err_t ret = i2c_master_cmd_begin(I2C_PORT_NUM,cmd,portMAX_DELAY);
    if(ret == ESP_ERR_TIMEOUT)
    {
        printf("SEND TimeOut ERROR CODE: %03X \n",ESP_ERR_TIMEOUT);
    }
    else if(ret == ESP_OK)
    {
        printf("SEND OKE \n");
    }
    else if(ret == ESP_FAIL)
    {
        printf("SEND FAIL ERROR CODE: %03X \n",ESP_FAIL);
    }
    else
    {
        printf("NO ACK, SENSOR, ERROR CODE: %s \n",esp_err_to_name(ret));
    }
    //step 7 delete link
    i2c_cmd_link_delete(cmd);
    return ret;
}