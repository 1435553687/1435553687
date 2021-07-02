#include "led.h"
#include "key.h"

#include "beep.h"
#include "delay.h"
#include "sys.h"

#include "usart.h"
#include "dht11.h"
#include "bh1750.h"
#include "timer.h"
#include "adc.h"
#include "oled.h"
#include "relay.h"
#include "exti.h"
#include "stdio.h"
#include "adc.h"

#include "esp8266.h"
#include "onenet.h"

u8 BeepFlag=0;//�Ƿ񱨾���־
u8 Beep_free=10;//�������Ƿ��ֶ�����

u8 humidityH; //ʪ����������
u8 humidityL; //ʪ��С������
u8 temperatureH; //�¶���������
u8 temperatureL; //�¶�С������

u8 LED_Status = 0;
u8 Chuan_Status = 0;
u8 Feng_Status = 0;

float Light=0; //���ն�
float Somg;
float ADCValue;
float ppm=0;
char PUB_BUF[256]; //�ϴ�����
const char *subtopic[] = {"/yyb/sub"};
const char pubtopic[]="/yyb/pub";

//ALIENTEK miniSTM32������ʵ��1
//�����ʵ��  
//����֧�֣�www.openedv.com
//������������ӿƼ����޹�˾
 int main(void)
 {
  unsigned short timeCount = 0;	//���ͼ������	
	unsigned char *dataPtr = NULL;
	 
	delay_init();	    	 //��ʱ������ʼ��
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);// �����ж����ȼ�����2		 
	LED_Init();		  	//��ʼ����LED���ӵ�Ӳ���ӿ�
	RELAY_Init();		  	//��ʼ����̵������ӵ�Ӳ���ӿ�
	KEY_Init();          	//��ʼ���밴�����ӵ�Ӳ���ӿ�
	EXTIX_Init();		//�ⲿ�жϳ�ʼ��
  BEEP_Init(); //��ʼ��BEEP
	DHT11_Init();
	Adc_Init();	
	BH1750_Init();
	Usart1_Init(115200); //debug����
	Usart2_Init(115200); //stm32��esp8266ͨѶ����
	OLED_Init();
	OLED_ColorTurn(0);//0������ʾ��1 ��ɫ��ʾ
  OLED_DisplayTurn(0);//0������ʾ 1 ��Ļ��ת��ʾ
	OLED_Clear();
	 
	 
	TIM2_Int_Init(4999,7199);//ʱ�ӳ�ʼ��
	TIM3_Int_Init(2499,7199);
	UsartPrintf(USART_DEBUG, " Hardware init OK\r\n");
	 
	ESP8266_Init();					//��ʼ��ESP8266
	while(OneNet_DevLink())			//����OneNET
	delay_ms(500);
	
	BEEP=0;//������ʾ����ɹ�
	delay_ms(250);
	BEEP=1;
	
	OneNet_Subscribe(subtopic, 1);
	
	while(1)
	{
		if(timeCount % 40 == 0)//1000ms / 25=40  һ��ִ��һ��
		{
			/*��ʪ�ȴ�������ȡ����*/
			DHT11_Read_Data(&humidityH,&humidityL,&temperatureH,&temperatureL);
			//UsartPrintf(USART_DEBUG,"�¶ȣ�%d.%d  ʪ�ȣ�%d.%d",temperatureH,temperatureL,humidityH,humidityL);
			/*���նȴ���*/
			ADCValue=Get_Adc_Average(ADC_Channel_1,15);
			Somg=3.3/4096*ADCValue;
			ppm = Somg*100;
			if (!i2c_CheckDevice(BH1750_Addr))
			{
				Light = LIght_Intensity();
				//UsartPrintf(USART_DEBUG,"��ǰ���ն�Ϊ��%.1f lx\r\n", Light);
			}
			
			if(Beep_free == 10)//����������Ȩ�Ƿ���� alarm_is_free == 10 ��ʼֵΪ10
			{
				if(ppm < 200)BeepFlag = 0;
				else BeepFlag = 1;
			}
			if(Beep_free < 10)Beep_free++;
    }
		if(++timeCount >= 200)									//5000ms / 25 = 2000 ���ͼ��5000ms
		{	
			LED_Status = GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_4);//��ȡLED0��״̬
			Chuan_Status = GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_11);//��ȡ����״̬
			Feng_Status = GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_12);//��ȡ���ȵ�״̬
			UsartPrintf(USART_DEBUG, "OneNet_Publish\r\n");
			sprintf(PUB_BUF, "{\"Hum\":%d.%d,\"Temp\":%d.%d,\"Light\":%.1f,\"Ppm\":%.1f,\"Led\":%d,\"Chuan\":%d,\"Feng\":%d,\"Beep\":%d}",
			humidityH,humidityL,temperatureH,temperatureL,Light,ppm,LED_Status?0:1,Chuan_Status?0:1,Feng_Status?0:1,BeepFlag);
			OneNet_Publish(pubtopic, PUB_BUF);
			
			timeCount = 0;
			ESP8266_Clear();
		}
		
		dataPtr = ESP8266_GetIPD(3);
		if(dataPtr != NULL)
		OneNet_RevPro(dataPtr);
		
		delay_ms(10);
	}
 }

