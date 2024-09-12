#include "led.h"
#include "delay.h"
#include "key.h"
#include "sys.h"
#include "lcd.h"
#include "usart.h"	  
#include "sram.h"
#include "string.h"
#include "usmart.h"	
#include "malloc.h"
 
/************************************************
 ALIENTEK精英STM32开发板实验34
 内存管理 实验 
 技术支持：www.openedv.com
 淘宝店铺：http://eboard.taobao.com 
 关注微信公众平台微信号："正点原子"，免费获取STM32资料。
 广州市星翼电子科技有限公司  
 作者：正点原子 @ALIENTEK
************************************************/

 
 int main(void)
 {	 
	u8 key;		 
 	u8 i=0;	    
	u8 *p=0;
	u8 *tp=0;
	u8 paddr[18];				//存放P Addr:+p地址的ASCII值
  
	delay_init();	    	 //延时函数初始化	  
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//设置中断优先级分组为组2：2位抢占优先级，2位响应优先级
	uart_init(115200);	 	//串口初始化为115200
 	LED_Init();		  			//初始化与LED连接的硬件接口
	KEY_Init();					//初始化按键
	LCD_Init();			   		//初始化LCD   
 	my_mem_init(SRAMIN);		//初始化内部内存池 
	POINT_COLOR=RED;			//设置字体为红色 
	LCD_ShowString(30,50,200,16,16,"ELITE STM32F103 ^_^");	
	LCD_ShowString(30,70,200,16,16,"MALLOC TEST");	
	LCD_ShowString(30,90,200,16,16,"ATOM@ALIENTEK");
	LCD_ShowString(30,110,200,16,16,"2015/1/20"); 
	LCD_ShowString(30,130,200,16,16,"KEY0:Malloc  KEY1:Free");
	LCD_ShowString(30,150,200,16,16,"KEY_UP:Write");

 	POINT_COLOR=BLUE;//设置字体为蓝色 
	LCD_ShowString(30,170,200,16,16,"SRAMIN");
	LCD_ShowString(30,190,200,16,16,"SRAMIN USED:   %");  
  	while(1)
	{	
		key=KEY_Scan(0);			//不支持连按	
		switch(key)
		{
			case 0:					//没有按键按下	
				break;
			case KEY0_PRES:			//KEY0按下
				p=mymalloc(SRAMIN,2048);//申请2K字节
				if(p!=NULL)sprintf((char*)p,"Memory Malloc Test%03d",i);//向p写入一些内容
				break;
			case KEY1_PRES:			//KEY1按下
				myfree(SRAMIN,p);	//释放内存
				p=0;				//指向空地址	  
				break;
			case WKUP_PRES:			//KEY UP按下 
				if(p!=NULL)
				{
					sprintf((char*)p,"Memory Malloc Test%03d",i);//更新显示内容 	 
					LCD_ShowString(30,250,200,16,16,p);			 //显示P的内容
				} 
				break; 
		}
		if(tp!=p)
		{
			tp=p;
			sprintf((char*)paddr,"P Addr:0X%08X",(u32)tp);
			LCD_ShowString(30,230,200,16,16,paddr);	//显示p的地址
			if(p)LCD_ShowString(30,250,200,16,16,p);//显示P的内容
		    else LCD_Fill(30,250,239,266,WHITE);	//p=0,清除显示
		}
		delay_ms(10);   
		i++;
		if((i%20)==0)//DS0闪烁.
		{ 
			LCD_ShowNum(30+96,190,my_mem_perused(SRAMIN),3,16);//显示内部内存使用率
 			LED0=!LED0;
 		}
	}	   
}

