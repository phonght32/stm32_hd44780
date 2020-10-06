#include "lcd_16x04.h"

BYTE *lcd_set_command(BYTE command)
{
    BYTE *arr_command = (BYTE *)malloc(2 * sizeof(BYTE));
    arr_command[0] = command & 0xF0;
    arr_command[1] = (command << 4) & 0xF0;
    return arr_command;
}
BYTE *lcd_set_char(BYTE character)
{
    BYTE *arr_command = (BYTE *)malloc(2 * sizeof(BYTE));
    arr_command[0] = character & 0xF0;
    arr_command[1] = (character << 4) & 0xF0;
    return arr_command;
}
void i2c_send_command(BYTE command)
{
    BYTE *data = lcd_set_command(command);
    BYTE data_upper = data[0];
    BYTE data_lower = data[1];
    BYTE data_send[4];
    data_send[0] = data_upper | 0x04;
    data_send[1] = data_upper;
    data_send[2] = data_lower | 0x04;
    data_send[3] = data_lower | 0x08;
    for (int i = 0; i < 4; i++)
    {
        int ret = master_write_byte(LCD_ADDRESS, data_send + i, 1);
    }
}

void i2c_send_char(BYTE character)
{
    BYTE *data = lcd_set_char(character);
    BYTE data_upper = data[0];
    BYTE data_lower = data[1];
    BYTE data_send[4];
    data_send[0] = data_upper | 0x0D;
    data_send[1] = data_upper | 0x09;
    data_send[2] = data_lower | 0x0D;
    data_send[3] = data_lower | 0x09;
    for (int i = 0; i < 4; i++)
    {
        master_write_byte(LCD_ADDRESS, data_send + i, 1);
    }
}

void i2c_send_string(char *string_data)
{
    while (*string_data != '\0')
    {
        i2c_send_char(*string_data);
        string_data++;
    }
}
void LCD_goto_XY(int X, int Y)
{
    if(Y>2 || Y<1) 
    {
        printf("wrong rows\n");
        return;
    }
    else
    {
        BYTE cmd=(Y==1)?0x80|X:0xC0|X; 
        i2c_send_command(cmd);
    }
}
void LCD_INIT()
{
    i2c_send_command(0x33);
    vTaskDelay(10 / portTICK_PERIOD_MS);

    i2c_send_command(0x32);
    vTaskDelay(50 / portTICK_PERIOD_MS);

    i2c_send_command(LCD_2_ROWS_5x7SIZE);
    vTaskDelay(50 / portTICK_PERIOD_MS);

    i2c_send_command(LCD_CLEAR);
    vTaskDelay(50 / portTICK_PERIOD_MS);

    i2c_send_command(LCD_CURSOR_NEXT_CHAR);
    vTaskDelay(50 / portTICK_PERIOD_MS);

    i2c_send_command(LCD_DISPLAY_ON_CURSOR_OFF);
    vTaskDelay(50 / portTICK_PERIOD_MS);

    i2c_send_command(LCD_CURSOR_BACK);
    vTaskDelay(50 / portTICK_PERIOD_MS);
}