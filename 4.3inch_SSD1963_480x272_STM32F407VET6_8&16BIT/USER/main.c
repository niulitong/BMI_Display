/************************************************************************************
//STM32连接引脚是指TFTLCD插槽或者插座引脚内部连接的STM32引脚
//=================================电源接线=======================================//
//     LCD模块                    STM32连接引脚
//      3.3V          --->             DC3.3V          //电源
//      GND           --->             GND              //电源地
//=============================液晶屏数据线接线===================================//
//本模块默认数据总线类型为16位并口总线(8位模式请使用DB0~DB7)
//     LCD模块                    STM32连接引脚
//      D0            --->            PD14        -|   
//      D1            --->            PD15         |  
//      D2            --->            PD0          | 
//      D3            --->            PD1          | 
//      D4            --->            PE7          |
//      D5            --->            PE8          |
//      D6            --->            PE9          |
//      D7            --->            PE10         |===>液晶屏16位并口数据信号
//      D8            --->            PE11         |
//      D9            --->            PE12         |
//      D10           --->            PE13         |
//      D11           --->            PE14         |
//      D12           --->            PE15         |
//      D13           --->            PD8          |
//      D14           --->            PD9          |
//      D15           --->            PD10        -|
//=============================液晶屏控制线接线===================================//
//     LCD模块 				            STM32连接引脚 
//      WR            --->            PD5             //液晶屏写数据控制信号
//      RD            --->            PD4             //液晶屏读数据控制信号
//      RS            --->            PD11            //液晶屏数据/命令控制信号
//      REST          --->          复位引脚（默认）  //液晶屏复位控制信号（也可选择PD13）
//      CS            --->            PD7             //液晶屏片选控制信号
//      LED_A         --->            PB15            //液晶屏背光控制信号
//===============================触摸屏触接线=====================================//
//如果模块不带触摸功能或者带有触摸功能，但是不需要触摸功能，则不需要进行触摸屏接线
//	   LCD模块                    STM32连接引脚 
//      T_IRQ         --->            PB1             //电容或电阻触摸屏触摸中断信号
//      T_DO          --->            PB2             //电阻触摸屏SPI总线读信号
//      T_DIN         --->            PC4             //电阻触摸屏SPI总线写信号或电容触摸屏IIC总线数据信号
//      T_CS          --->            PC13            //电阻触摸屏片选控制信号或电容触摸屏复位信号
//      T_CLK         --->            PB0             //电阻触摸屏SPI总线或电容触摸屏IIC总线时钟信号
*************************************************************************************/		
#include "delay.h"
#include "sys.h"
#include "lcd.h"
#include "touch.h"
#include "gui.h"
#include "test.h"
#include "led.h"

// 1. 棋盘格测试：检测数据线干扰 (Crosstalk)
// 如果看到画面闪烁、有杂点或横线，说明排母处的信号干扰严重
void Test_Checkerboard(void) {
    uint32_t i;
    LCD_SetWindows(0, 0, 480-1, 272-1);
    for (i = 0; i < 480 * 272; i++) {
        // 每隔一个点变换颜色
        if ((i % 2 == 0)) LCD_WR_DATA(0xFFFF); // 白色
        else LCD_WR_DATA(0x0000);             // 黑色
    }
}

// 2. 纯色压力测试：检测位权错误 (Bit Error)
// 依次刷 红、绿、蓝。
// 如果红屏上有细微绿点，说明数据线 D5-D10 某根线与 D11-D15 有串扰
void Test_ColorBurst(uint16_t color) {
    uint32_t i;
    LCD_SetWindows(0, 0, 480-1, 272-1);
    for (i = 0; i < 480 * 272; i++) {
        LCD_WR_DATA(color);
    }
}

// 3. 逐行扫描测试：检测行同步 (HSYNC/Porch)
// 如果线条不是笔直的，或者在移动时发生断裂，说明 SSD1963 的时序参数不对
void Test_MovingLine(uint16_t y_pos) {
    uint16_t x;
    LCD_SetWindows(0, y_pos, 480-1, y_pos); // 只刷一行
    for (x = 0; x < 480; x++) {
        LCD_WR_DATA(0xF800); // 红色行
    }
}
int main(void)
{	
	uint16_t line = 0;
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//设置系统中断优先级分组2
	delay_init(168);     //初始化延时函数
	delay_ms(1000);
	LED_Init();
	LCD_Init();	   //液晶屏初始化
  //循环测试
	//while(1)
	//{
		//main_test(); 		//测试主界面
		//Test_Read();     //读ID和颜色值测试
		//Test_Color();  		//简单刷屏填充测试
		//Test_FillRec();		//GUI矩形绘图测试
		//Test_Circle(); 		//GUI画圆测试
		//Test_Triangle();    //GUI三角形绘图测试
		//English_Font_test();//英文字体示例测试
		//Chinese_Font_test();//中文字体示例测试
		//Pic_test();			//图片显示示例测试
		//Test_Dynamic_Num();  //动态数字显示
		//Rotate_Test();   //旋转显示测试
		//如果不带触摸，或者不需要触摸功能，请注释掉下面触摸屏测试项
		//Touch_Test();		//触摸屏手写测试  
	//}
	/* USER CODE BEGIN WHILE */
	GPIO_SetBits(GPIOF, GPIO_Pin_9);

while (1)
{
	// 每次切换测试模式时，翻转一次 LED 状态
    GPIO_ToggleBits(GPIOD, GPIO_Pin_12); // 假设 LED_RED 在 PD12，请改为你板子上的实际引脚
    
    Test_ColorBurst(0xF800); 
    delay_ms(1000);
    
    Test_ColorBurst(0x07E0); 
    delay_ms(1000);
    // --- 模式 1: 颜色压力测试 (每隔 1秒换一种三原色) ---
    Test_ColorBurst(0xF800); // 纯红
    delay_ms(1000);
    Test_ColorBurst(0x07E0); // 纯绿
    delay_ms(1000);
    Test_ColorBurst(0x001F); // 纯蓝
    delay_ms(1000);

    // --- 模式 2: 棋盘格测试 (高频信号完整性测试) ---
    Test_Checkerboard();
    delay_ms(2000);
    
    // --- 模式 3: 动态扫描 (测试同步) ---
     LCD_Clear(WHITE); 
     for(line=0; line<272; line++) {
        Test_MovingLine(line);
        delay_ms(5);
     }
    
    /* USER CODE END WHILE */
}
}

