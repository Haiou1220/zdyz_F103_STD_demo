#include "timer.h"
#include "led.h"
#include "led.h"
#include "usart.h"
#include "malloc.h"
#include "string.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK战舰STM32开发板
//定时器 驱动代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//修改日期:2012/9/3
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2009-2019
//All rights reserved									  
//////////////////////////////////////////////////////////////////////////////////   	 

//FreeRTOS时间统计所用的节拍计数器
volatile unsigned long long FreeRTOSRunTimeTicks;

//初始化TIM3使其为FreeRTOS的时间统计提供时基
void ConfigureTimeForRunTimeStats(void)
{
	//定时器3初始化，定时器时钟为72M，分频系数为72-1，所以定时器3的频率
	//为72M/72=1M，自动重装载为50-1，那么定时器周期就是50us
	FreeRTOSRunTimeTicks=0;
	TIM3_Int_Init(50-1,72-1);	//初始化TIM3
}

//通用定时器3中断初始化
//这里时钟选择为APB1的2倍，而APB1为36M
//arr：自动重装值。
//psc：时钟预分频数
//这里使用的是定时器3!
void TIM3_Int_Init(u16 arr,u16 psc)
{
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE); //时钟使能
	
	//定时器TIM3初始化
	TIM_TimeBaseStructure.TIM_Period = arr; //设置在下一个更新事件装入活动的自动重装载寄存器周期的值	
	TIM_TimeBaseStructure.TIM_Prescaler =psc; //设置用来作为TIMx时钟频率除数的预分频值
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //设置时钟分割:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM向上计数模式
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure); //根据指定的参数初始化TIMx的时间基数单位
 
	TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE ); //使能指定的TIM3中断,允许更新中断

	//中断优先级NVIC设置
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;  //TIM3中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;  //先占优先级4级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;  //从优先级0级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQ通道被使能
	NVIC_Init(&NVIC_InitStructure);  //初始化NVIC寄存器

	TIM_Cmd(TIM3, ENABLE);  //使能TIMx					 
}

//通用定时器2中断初始化
//这里时钟选择为APB1的2倍，而APB1为36M
//arr：自动重装值。
//psc：时钟预分频数
//这里使用的是定时器2!
void TIM2_Int_Init(u16 arr,u16 psc)
{
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE); //时钟使能
	
	//定时器TIM2初始化
	TIM_TimeBaseStructure.TIM_Period = arr; 			//设置在下一个更新事件装入活动的自动重装载寄存器周期的值	
	TIM_TimeBaseStructure.TIM_Prescaler =psc; 			//设置用来作为TIMx时钟频率除数的预分频值
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //设置时钟分割:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM向上计数模式
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure); 		//根据指定的参数初始化TIMx的时间基数单位
 
	TIM_ITConfig(TIM2,TIM_IT_Update,ENABLE ); 			//使能指定的TIM2中断,允许更新中断

	//中断优先级NVIC设置
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;  	//TIM2中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 8;  //先占优先级4级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;  //从优先级0级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; 	//IRQ通道被使能
	NVIC_Init(&NVIC_InitStructure);  					//初始化NVIC寄存器

	TIM_Cmd(TIM2, ENABLE);  							//使能TIM2		 
}

//定时器3中断服务函数
void TIM3_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM3,TIM_IT_Update)==SET) //溢出中断
	{
		FreeRTOSRunTimeTicks++;
	}
	TIM_ClearITPendingBit(TIM3,TIM_IT_Update);  //清除中断标志位
}

extern QueueHandle_t Message_Queue;	//信息队列句柄
extern void disp_str(u8* str);

//定时器2中断服务函数
void TIM2_IRQHandler(void)
{
	u8 *buffer;
	BaseType_t xTaskWokenByReceive=pdFALSE;
	BaseType_t err;
	
	if(TIM_GetITStatus(TIM2,TIM_IT_Update)==SET) //溢出中断
	{
		buffer=mymalloc(SRAMIN,USART_REC_LEN);
        if(Message_Queue!=NULL)
        {
			memset(buffer,0,USART_REC_LEN);	//清除缓冲区
			err=xQueueReceiveFromISR(Message_Queue,buffer,&xTaskWokenByReceive);//请求消息Message_Queue
            if(err==pdTRUE)			//接收到消息
            {
				disp_str(buffer);	//在LCD上显示接收到的消息
            }
        }
		myfree(SRAMIN,buffer);		//释放内存
		
		portYIELD_FROM_ISR(xTaskWokenByReceive);//如果需要的话进行一次任务切换
	}
	TIM_ClearITPendingBit(TIM2,TIM_IT_Update);  //清除中断标志位
}



