#ifndef __LCD_DRIVER_H__
#define __LCD_DRIVER_H__

#include <stdint.h>

//画笔颜色 rbg
#define WHITE         	 0xFFFF
#define BLACK         	 0x0000
#define RED           	 0xF800
#define GREEN         	 0x07E0
#define BLUE         	 0x001F
#define GRED 			 0XFFE0
#define BRED             0XF81F
#define GBLUE			 0X07FF
#define MAGENTA       	 0xF81F
#define CYAN          	 0x7FFF
#define YELLOW        	 0xFFE0
#define BROWN 			 0XBC40 //棕色
#define BRRED 			 0XFC07 //棕红色
#define GRAY  			 0X8430 //灰色
//GUI颜色

#define DARKBLUE      	 0X01CF	//深蓝色
#define LIGHTBLUE      	 0X7D7C	//浅蓝色  
#define GRAYBLUE       	 0X5458 //灰蓝色
//以上三色为PANEL的颜色 
 
#define LIGHTGREEN     	 0X841F //浅绿色
#define LGRAY 			 0XC618 //浅灰色(PANNEL),窗体背景色

#define LGRAYBLUE        0XA651 //浅灰蓝色(中间层颜色)
#define LBBLUE           0X2B12 //浅棕蓝色(选择条目的反色)

void InitLCD(void);
void ShowString(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t size, char *p);

#endif // __LCD_DRIVER_H__
