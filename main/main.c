#include "i2c_protocol.h"
#include "lcd_16x04.h"
static void test_LCD()
{

  while (1)
  {
    i2c_send_command(LCD_CLEAR);
    vTaskDelay(1000/portTICK_PERIOD_MS);
    LCD_goto_XY(0,1);
    i2c_send_string("I am an engineer");
    LCD_goto_XY(0,2);
    i2c_send_string("here is my LCD");
    vTaskDelay(1000/portTICK_PERIOD_MS);

  }
}
void app_main(void)
{
  i2c_master_init();
  printf("i2c_master_init success \n");
  LCD_INIT();
  printf("lcd_init success \n");
  LCD_goto_XY(0, 1);
  i2c_send_string("Hello World");
  vTaskDelay(500 / portTICK_PERIOD_MS);
  xTaskCreate(test_LCD, "test", 1024, NULL, 10, NULL);
  printf("lcd_string success \n");
}
