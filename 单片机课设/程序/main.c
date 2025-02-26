#include <reg52.h>
#include <absacc.h>
#include<intrins.h>
#include"temp.h"
#define Contadd XBYTE[0xff23] 		//8255控制地址
#define PA XBYTE[0xff20]        	//段选信号地址
#define PB XBYTE[0xff21] 	 	//片选信号地址 
#define adc XBYTE [0xFF80] 		//片选指针，A/D转换电路片外地址
#define uchar unsigned char		//宏定义	
#define uint unsigned int
typedef unsigned int u16;	  //对数据类型进行声明定义
typedef unsigned char u8;	

sbit DJ_A=P1^0;		//电机的4个相位
sbit DJ_B=P1^1;
sbit DJ_C=P1^2;
sbit DJ_D=P1^3;

int DJ_X = 0;		//电机相角判断
long int JD = 0;	//电机角度

sbit RUN=P3^2;		//启停开关
sbit K1=P1^4;			//模式选择
sbit K2=P1^5;			//左移
sbit K3=P1^6;			//右移
sbit K4=P1^7;			//加
sbit K5=P3^0;			//减
sbit K6=P3^1;			//确定

int LV0 = 0;			//数码管显示速度
int V0 = 0;				//速度

int st=200;				//数码管闪烁
int sj = 0;				//是否调数

int direct=0;			//转向判断
int direct0=0;
int mode = 0;			//模式
int go = 0;				//是否启动

long int JDC=0;
int JDC_W = 1;		//角度旋转标致
int JCC_LED1=0;		//电机角度LED显示
int JCC_LED2=0;
int JCC_LED3=0;
int JCC_LED4=0;
int JCC_LED5=0;
int wv=60;				//循环位

char num=0;
unsigned char DisplayData[4];	//温度数据
int T_judge=0;								//温度滤波

int TH_ = 0x4C;		//设置初脉宽0.02s
int TL_ = 0x00;

unsigned char code smgduan[17]={0xc0,0xf9,0xa4,0xb0,0x99,0x92,0x82,0xf8,
				0x80,0x90,0x88,0x83,0xc6,0xa1,0x86,0x8e};//显示0~F的值


/*******************************************************************************
* 函 数 名 				: delay
* 函数功能				: 延时函数，i=1时，大约延时10us
*******************************************************************************/
void delay(unsigned int i)
{
	while(i--);	
}

/*******************************************************************************
* 函 数 名     		: T0_init
* 函数功能				: 定时器T0初始化，初定计时0.02s
*******************************************************************************/
void T0_init(void)
{
	TMOD|=0X0;	//工作方式1，计时使用
	TH0=TH_;	//初值
	TL0=TL_;	
	ET0=1;
}

/*******************************************************************************
* 函 数 名        : Int0Init
* 函数功能		   	: 外部中断INT0初始化，下降沿触发
*******************************************************************************/
void Int0Init()
{
	//设置INT0
	IT0=1;//跳变沿出发方式（下降沿）
	EX0=1;//打开INT0的中断允许。	
}

/*******************************************************************************
* 函 数 名        : datapros
* 函数功能		   	: 温度读取转换函数
* 输入						: 温度数值temp
*******************************************************************************/
void datapros(int temp) 	 
{
   	float tp;  
	if(temp< 0)				//当温度值为负数
  	{
		DisplayData[0] = 0xBF; 	  //   -
		//因为读取的温度是实际温度的补码，所以减1，再取反求出原码
		temp=temp-1;
		temp=~temp;
		tp=temp;
		temp=tp*0.0625*100+0.5;	
		//留两个小数点就*100，+0.5是四舍五入，因为C语言浮点数转换为整型的时候把小数点
		//后面的数自动去掉，不管是否大于0.5，而+0.5之后大于0.5的就是进1了，小于0.5的就
		//算加上0.5，还是在小数点后面。
 
  	}
 	else
  	{			
		DisplayData[0] = 0xFF;
		tp=temp;//因为数据处理有小数点所以将温度赋给一个浮点型变量
		//如果温度是正的那么，那么正数的原码就是补码它本身
		temp=tp*0.0625*100+0.5;	
		//留两个小数点就*100，+0.5是四舍五入，因为C语言浮点数转换为整型的时候把小数点
		//后面的数自动去掉，不管是否大于0.5，而+0.5之后大于0.5的就是进1了，小于0.5的就
		//算加上0.5，还是在小数点后面。
	}
	DisplayData[0] = smgduan[temp / 10000];
	DisplayData[1] = smgduan[temp % 10000 / 1000];
	DisplayData[2] = smgduan[temp % 1000 / 100] & 0x7F;
	DisplayData[3] = smgduan[temp % 100 / 10];
	if(temp/10>=315){				//高温报警
		T_judge+=1;
	}
	else T_judge=0;
	if(T_judge>10){
		TR0 = 0;
		DJ_A=1;
		//PB &= 0x7F;
	}
}

/*******************************************************************************
* 函 数 名        : SMG_Display
* 函数功能		   	: 数码管显示函数，依次显示5种模式，即1方向、2速度档位、3角度、
									  4设定旋转固定角度、5温度显示
*******************************************************************************/
void SMG_Display()
{
	int i;
	for(i=0;i<6;i++)
	{
		switch(i)	 //位选，选择点亮的数码管，
		{
			case(0):
				PA=0xDF; break;//显示第0位
			case(1):
				PA=0xEF; break;//显示第1位
			case(2):
				PA=0xF7; break;//显示第2位
			case(3):
				PA=0xFB; break;//显示第3位
			case(4):
				PA=0xFD; break;//显示第4位
			case(5):
				PA=0xFE; break;//显示第5位
		}
		if(mode%5==0){
			switch(i){
				case(5):if(sj%2==0||st<100)PB=smgduan[direct%2];else PB=0xFF; break;
				case(4):PB=0xFF;break;
				case(3):PB=0xFF;break;
				case(2):PB=0xFF;break;
				case(1):PB=0xFF;break;
				case(0):PB=0xA1;break;
			}
			delay(100); //间隔一段时间扫描
			PB=0xFF;//消隐
			if(sj%2==1)
				st = st-1;
			if(st==0){st = 200;}
		}
		if(mode%5==1){
			switch(i){
				case(5):if(sj%2==1&&(st>=100)) PB=0xFF;else PB=smgduan[LV0];break;
				case(4):PB=0xFF;break;
				case(3):PB=0xFF;break;
				case(2):PB=0xFF;break;
				case(1):PB=0xFF;break;
				case(0):PB=0xC1;break;
			}
			delay(100); //间隔一段时间扫描	
			PB=0xFF;//消隐
			if(sj%2==1)
				st = st-1;
			if(st==0){st = 200;}
		}
		if(mode%5==2){
			switch(i){
				case(0):if(sj%2==1&&(st>=100)) PB=0xFF; else if(JD/10000==0);else PB=smgduan[JD/10000];break;
				case(1):if(sj%2==1&&(st>=100)) PB=0xFF; else if(JD/1000==0);else PB=smgduan[(JD-JD/10000*10000)/1000];break;
				case(2):if(sj%2==1&&(st>=100)) PB=0xFF; else PB=smgduan[(JD-JD/1000*1000)/100]&0x7F;break;
				case(3):if(sj%2==1&&(st>=100)) PB=0xFF; else PB=smgduan[(JD-JD/100*100)/10];break;
				case(4):if(sj%2==1&&(st>=100)) PB=0xFF; else PB=smgduan[JD%10];break;
				case(5):PB=0x9C;break;
			}
			delay(100); //间隔一段时间扫描	
			PB=0xFF;//消隐
			if(sj%2==1)
				st = st-1;
			if(st==0){st = 200;}
		}
		if(mode%5==3){
			switch(i){
				case(0):if(sj%2==1&&(st>=100)&&(wv%5==4)) PB=0xFF; else PB=smgduan[JCC_LED5];break;
				case(1):if(sj%2==1&&(st>=100)&&(wv%5==3)) PB=0xFF; else PB=smgduan[JCC_LED4];break;
				case(2):if(sj%2==1&&(st>=100)&&(wv%5==2)) PB=0xFF; else PB=smgduan[JCC_LED3]&0x7F;break;
				case(3):if(sj%2==1&&(st>=100)&&(wv%5==1)) PB=0xFF; else PB=smgduan[JCC_LED2];break;
				case(4):if(sj%2==1&&(st>=100)&&(wv%5==0)) PB=0xFF; else PB=smgduan[JCC_LED1];break;
				case(5):PB=0x94;break;
			}
			delay(100); //间隔一段时间扫描	
			PB=0xFF;//消隐
			if(sj%2==1)
				st = st-1;
			if(st==0){st = 200;}
		}
		if(mode%5==4){
			switch(i){
				case(0):PB=DisplayData[0];break;
				case(1):PB=DisplayData[1];break;
				case(2):PB=DisplayData[2];break;
				case(3):PB=DisplayData[3];break;
				case(4):PB=0x9C;break;
				case(5):PB=0xC6;break;
			}
			delay(100); //间隔一段时间扫描	
			PB=0xFF;//消隐
		}
	}
}

/*******************************************************************************
* 函 数 名        : calculate_time
* 函数功能		   	: 由时间档位转换定时时间
*******************************************************************************/
void calculate_time()
{
	switch(V0){
		case(0):TH_ = 0x4C;TL_ = 0x00;break;
		case(1):TH_ = 0x70;TL_ = 0x00;break;
		case(2):TH_ = 0x94;TL_ = 0x00;break;
		case(3):TH_ = 0xB8;TL_ = 0x00;break;
	}
}

/*******************************************************************************
* 函 数 名        : DATA_get
* 函数功能		   	: 或取输入的按键
*******************************************************************************/
void DATA_get()	//按键输入数据
{
	if(K1==0)		  //模式转换
	{	
		delay(1000);
		if(K1==0)
		{
			mode = mode+1;	//模式修改
			sj = 0;					//禁止修改数字
		}
		while(!K1);
	}
	if(K6==0)		  //是否改变参数
	{	
		delay(1000);
		if(K6==0)
		{
			sj = sj+1;										//可修改参数与不可修改参数
		}
		if(sj%2==0&&(mode%5==1))				//模式2情况
		{
			V0=LV0;										//速度档位更新
			calculate_time();					//计算新的定时时间
		}
		else if(sj%2==0&&(mode%5==0)){	//模式1情况
			direct0 = direct;					//修改方向
		}
		else if(sj%2==0&&(mode%5==3)){	//模式4情况
			JDC=JCC_LED1+10*JCC_LED2+100*JCC_LED3+1000*JCC_LED4+10000*JCC_LED5;
			TR0=1;										//开定时器
			JDC_W = 0;								//判断标致
		}
		while(!K6);
	}
	
	if((mode%5)==0&&(sj%2)==1)	//改变正转反转
	{
		if(K4==0)								//增加
		{
			delay(1000);
			if(K4==0)
			{
				direct = direct+1;
			}
			while(!K4);
		}
		if(K5==0)								//减少
		{
			delay(1000);
			if(K5==0)
			{
				direct = direct+1;
			}
			while(!K5);
		}
	}
	if(mode%5==1&&(sj%2)==1)	//改变转速
	{
		if(K4==0)		//加操作
		{
			delay(1000);
			if(K4==0)
			{
				LV0=LV0+1;
				if(LV0==4)
					LV0=0;
			}
			while(!K4);
		}
		if(K5==0)		//减操作
		{
			delay(1000);
			if(K5==0)
			{
				LV0=LV0-1;
				if(LV0==-1)
					LV0=3;
			}
			while(!K5);
		}
	}
	if((mode%5)==2&&(sj%2)==1)	//校正角度
	{
		if(K4==0)
		{
			delay(1000);
			if(K4==0)
			{
				JD=0;
			}
			while(!K4);
		}
		if(K5==0)
		{
			delay(1000);
			if(K5==0)
			{
				JD=0;
			}
			while(!K5);
		}
	}
	if(mode%5==3&&(sj%2)==1)	//改变初设角
	{
		if(K2==0)
		{
			delay(1000);
			if(K2==0)
			{
				wv=wv+1;
			}
			while(!K2);
		}
		if(K3==0)
		{
			delay(1000);
			if(K3==0)
			{
				wv=wv-1;
			}
			while(!K3);
		}
		if(K4==0)		//加操作
		{
			delay(1000);
			if(K4==0)
			{
				if(wv%5==0){			//数码管每一位操作
					JCC_LED1+=1;
					if(JCC_LED1==10)
						JCC_LED1=0;
				}
				if(wv%5==1){
					JCC_LED2+=1;
					if(JCC_LED2==10)
						JCC_LED2=0;
				}
				if(wv%5==2){
					JCC_LED3+=1;
					if(JCC_LED3==10)
						JCC_LED3=0;
				}
				if(wv%5==3){
					JCC_LED4+=1;
					if(JCC_LED4==10)
						JCC_LED4=0;
				}
				if(wv%5==4){
					JCC_LED5+=1;
					if(JCC_LED5==10)
						JCC_LED5=0;
				}
			}
			while(!K4);
		}
		if(K5==0)		//减操作
		{
			delay(1000);
			if(K5==0)
			{
				if(wv%5==0){			//数码管每一位操作
					JCC_LED1-=1;
					if(JCC_LED1==-1)
						JCC_LED1=9;
				}
				if(wv%5==1){
					JCC_LED2-=1;
					if(JCC_LED2==-1)
						JCC_LED2=9;
				}
				if(wv%5==2){
					JCC_LED3-=1;
					if(JCC_LED3==-1)
						JCC_LED3=9;
				}
				if(wv%5==3){
					JCC_LED4-=1;
					if(JCC_LED4==-1)
						JCC_LED4=9;
				}
				if(wv%5==4){
					JCC_LED5-=1;
					if(JCC_LED5==-1)
						JCC_LED5=9;
				}
			}
			while(!K5);
		}
	}
}

/*******************************************************************************
* 函 数 名        : main
* 函数功能		   	: 主函数
*******************************************************************************/
void main()
{	
	DJ_A=1;					//固定住电机
	Contadd=0x81;		//设定8255的工作方式：PA、PB口为输出口,PC口为输入口
	T0_init();			//设置定时器0中断
	Int0Init();   	//设置外部中断0
	TR0=0;					//禁止定时器
	EA = 1;					//开总中断
	while(1){
		DATA_get();											//获取按键
		datapros(Ds18b20ReadTemp());	  //温度数据处理
		SMG_Display();									//数码管显示
	}
}

/*******************************************************************************
* 函 数 名        : Int0()	interrupt 0
* 函数功能		   	: 外部中断0的中断服务程序
*******************************************************************************/
void Int0()	interrupt 0
{
	TR0=~TR0;			//修改电机工作状态
	DJ_A=1;				//电机从A相位开始
	DJ_X=0;				//相位修改
}

/*******************************************************************************
* 函 数 名        : Timer0() interrupt 1
* 函数功能		   	: 定时器中断0的中断服务程序
*******************************************************************************/
void Timer0() interrupt 1	//定时器中断
{
	if(JDC>0||JDC_W==1){	//满足预定角度>0或者电机旋转标志=1可启动电机
		if(direct0%2==0){		//正传
			switch(DJ_X){
				case(0):DJ_A=1;DJ_B=0;DJ_C=0;DJ_D=0;break;
				case(1):DJ_A=1;DJ_B=1;DJ_C=0;DJ_D=0;break;
				case(2):DJ_A=0;DJ_B=1;DJ_C=0;DJ_D=0;break;
				case(3):DJ_A=0;DJ_B=1;DJ_C=1;DJ_D=0;break;
				case(4):DJ_A=1;DJ_B=0;DJ_C=1;DJ_D=0;break;
				case(5):DJ_A=0;DJ_B=0;DJ_C=1;DJ_D=1;break;
				case(6):DJ_A=0;DJ_B=0;DJ_C=0;DJ_D=1;break;
				case(7):DJ_A=1;DJ_B=0;DJ_C=0;DJ_D=1;break;
			}
			JD=JD+9;					//绝对角度
			JD=JD%36000;		
		}
		else{								//反转
			switch(DJ_X){
				case(0):DJ_A=1;DJ_B=0;DJ_C=0;DJ_D=0;break;
				case(1):DJ_A=1;DJ_B=0;DJ_C=0;DJ_D=1;break;
				case(2):DJ_A=0;DJ_B=0;DJ_C=0;DJ_D=1;break;
				case(3):DJ_A=0;DJ_B=0;DJ_C=1;DJ_D=1;break;
				case(4):DJ_A=0;DJ_B=0;DJ_C=1;DJ_D=0;break;
				case(5):DJ_A=0;DJ_B=1;DJ_C=1;DJ_D=0;break;
				case(6):DJ_A=0;DJ_B=1;DJ_C=0;DJ_D=0;break;
				case(7):DJ_A=1;DJ_B=1;DJ_C=0;DJ_D=0;break;
			}
			JD=JD-9;					//绝对角度
			if(JD<=0) JD+=36000;
		}
		DJ_X = (DJ_X + 1)%8;	//相位修改
		if(JDC_W==0){				//如果模式4情况
			JDC=JDC-9;				//修改预定角
			if(JDC<0){				//预定角已到达
				JDC_W=1;				//修改标志
				TR0=0;					//关闭定时器，电机停�
				DJ_A=1;					//电机相位
			}
		}
	}
	TH0=TH_;	//给定时器赋初值，定时1ms
	TL0=TL_;
}