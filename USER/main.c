#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "beep.h"
#include "lcd.h"
#include "key.h"
#include "malloc.h"
#include "w25qxx.h"
#include "RC522.h"
#include "spi.h"
#include "string.h"
#include "timer.h"
#include "main.h"
#include "exti.h"
// #include "sram.h"
// #include "usmart.h"
// #include "sdio_sdcard.h"
// #include "ff.h"
// #include "exfuns.h"
// #include "fontupd.h"
// #include "text.h"
// #include "rtc.h"

/** 接线方式
	PC10 -- CLK
	PC11 -- MISO
	PC12 -- MOSI
	PA4  -- NSS
	PA6  -- RST
*/

u8 CardID[ID_LEN];	  // 存放ID卡号
u8 keyFun = KEY_NULL; // 先初始化没有任何按键按下的状态

/* 定义FLASH有关变量 当FLASH_START = FLASH_SIZE时 则为存储容量已满 */
const u8 START_ADDR = 10;			  // FLASH存储的起始地址
const u32 FLASH_SIZE = 16 * 1024 * 8; // FLASH大小为16KB

int main(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //设置系统中断优先级分组2
	delay_init(168);								//初始化延时函数
	LED_Init();										//初始化LED
	BEEP_Init();									//初始化蜂鸣器
	KEY_Init();										//初始化按键
	EXTIX_Init();									// 初始化外部中断 用于读取按键值
	W25QXX_Init();									//初始化W25Q128
	uart_init(115200);								//串口初始化波特率为115200
	LCD_Init();										//LCD初始化
	RC522_SPI3_Init();
	MFRC522_Initializtion();

	POINT_COLOR = RED; // 设置字体为红色
	/* 检测W25Q128 */
	while (W25QXX_ReadID() != W25Q128)
	{
		LCD_ShowString(30, 90, 200, 16, 16, "W25Q128 Check Failed!");
		delay_ms(500);
		LCD_ShowString(30, 90, 200, 16, 16, "Please Check!      ");
		delay_ms(500);
		LED0 = !LED0; // DS0闪烁
	}
	LCD_ShowString(30, 90, 200, 16, 16, "W25Q128 Ready!");
	LCD_ShowString(30, 110, 200, 16, 16, "System loading...");
	delay_ms(3000);
	DCSInit();	  // DoorControlSystem(DCS)初始化
	DCSRunning(); // DoorControlSystem(DCS)运行
}

/**
 * @brief DoorControlSystem(DCS)运行
 */
void DCSRunning(void)
{
	u8 status;
	while (1)
	{
		LcdDesktop();
		switch (keyFun)
		{
		case KEY0_VALUE:
			UserLogin();	   // 用户登录 -> 读卡信息
			keyFun = KEY_NULL; // 置为KEY_NULL -> 防止下次循环继续调用此功能
			break;
		case KEY1_VALUE:
			AddUserID();	   // 添加用户 添加用户中也需要读卡
			keyFun = KEY_NULL; // 置为KEY_NULL -> 防止下次循环继续调用此功能
			break;
		case KEY2_VALUE:
			status = ReadCard(); // 读卡 此按键只是为了测试是否能正确读卡
			if (status == MI_OK)
			{
				OK_BEEP();
			}
			keyFun = KEY_NULL; // 置为KEY_NULL -> 防止下次循环继续调用此功能
			break;
		}
	}
}

/**
 * @brief 显示LCD桌面
 */
void LcdDesktop(void)
{
	// LCD_Clear(WHITE);
	LCD_ShowString(20, 150, 200, 16, 16, "=========================");
	LCD_ShowString(20, 170, 200, 16, 16, "*       <Desktop>       *");
	LCD_ShowString(20, 190, 200, 16, 16, "=========================");
	LCD_ShowString(20, 210, 200, 16, 16, "*    KEY0 <Running>     *");
	LCD_ShowString(20, 230, 200, 16, 16, "*    KEY1 <Add User>    *");
	LCD_ShowString(20, 250, 200, 16, 16, "*    KEY2 <Read Card>   *");
	LCD_ShowString(20, 270, 200, 16, 16, "=========================");
}

/**
 * @brief 用户登录
 */
void UserLogin(void)
{
	u8 status;
	while (1)
	{
		status = ReadCard();
		if (status == MI_OK)
		{
			/* 用户卡ID存在 合法用户 */
			if (SerachID(CardID) == EXIST)
			{
				LCD_ShowString(80, 110, 200, 16, 16, "Open door!");
				/* 读卡成功提示 */
				OK_BEEP();
				OK_LED();
			}
			else /* 用户卡ID不存在 非法用户 */
			{
				LCD_ShowString(80, 110, 200, 16, 16, "Error!");
				/* 读卡错误提示 */
				ERR_BEEP();
				ERR_LED();
			}
		}
		else /* 读卡失败 重新读卡 */
		{
			LCD_Clear(BLUE);
			LCD_ShowString(80, 110, 200, 16, 16, "Read card again!");
			ERR_BEEP();
			delay_ms(1000);
		}
		/* 用户可强制退出运行模式 */
		if (keyFun == KEYUP_VALUE)
		{
			return;
		}
	}
}

/**
 * @brief 在数据存储区搜索用户卡的ID
 * @param CardID[u8*] 用户卡ID
 * @return u32 
 * 若用户卡ID存在 则返回返回用户卡ID已存在标志
 * 若用户卡ID不存在 则返回当前可存入ID的数据存储区的地址
 * 若数据存储区已满 返回FLASH_SIZE
 */
u32 SerachID(u8 *ID)
{
	u8 IDValue[ID_LEN];
	u8 i;
	for (i = START_ADDR; i < FLASH_SIZE; i += 4)
	{
		W25QXX_Read(IDValue, i, sizeof(ID));

		/* 数据存储区中无此用户卡ID */
		if (IDValue[0] == 0x0 && IDValue[1] == 0x0 && IDValue[2] == 0x0 && IDValue[3] == 0x0)
		{
			return i; // 返回当前可存入数据存储区的地址
		}
		/* 判断读取到的ID是否正确 */
		if (IsEquals(IDValue, ID) == TRUE)
		{
			return EXIST; // 返回用户卡ID已存在标志
		}
	}
	return FLASH_SIZE; // 数据存储区已满
}

/**
 * @brief 比较两个数组是否相等
 * @param arr1[u8*]
 * @param arr2[u8*] 
 * @return Boolean 相等返回TRUE 否则返回FALSE
 */
Boolean IsEquals(u8 *arr1, u8 *arr2)
{
	u8 i;
	u8 ans = 0;
	u8 len = sizeof(arr1);

	if (len != sizeof(arr2))
	{
		return FALSE;
	}

	for (i = 0; i < len; i++)
	{
		ans |= arr1[i] ^ arr2[i];
	}

	return ans == 0 ? TRUE : FALSE;
}

/**
 * @brief 返回给定模式的时间 或 显示时间
 * @param timeMode[u8] 选择的模式
 * @return u8 D返回日 H返回时、M返回分  DHM显示时间，无返回
 */
u8 ShowTime(u8 timeMode)
{
	RTC_TimeTypeDef TheTime; //时间
	RTC_DateTypeDef TheData; //日期
	u8 *TimeBuff;			 //时间存储

	RTC_GetTime(RTC_Format_BIN, &TheTime);
	RTC_GetDate(RTC_Format_BIN, &TheData);
	switch (timeMode)
	{
	case D:
		return TheData.RTC_Date;
	case H:
		return TheTime.RTC_Hours;
	case M:
		return TheTime.RTC_Minutes;
	case DHM:
		TimeBuff = malloc(40);
		sprintf((char *)TimeBuff, "%02d%s:%02d%s:%02d%s", TheData.RTC_Date, "D", TheTime.RTC_Hours, "H", TheTime.RTC_Minutes, "M");
		LCD_ShowString(130, 300, 200, 16, 16, TimeBuff);
		free(TimeBuff);
		break;
	}
	return 0;
}

/**
 * @brief 显示添加用户的菜单
 */
void ShowMenu(void)
{
	LCD_ShowString(20, 150, 200, 16, 16, "=========================");
	LCD_ShowString(20, 170, 200, 16, 16, "*       <Add User>      *");
	LCD_ShowString(20, 190, 200, 16, 16, "=========================");
	LCD_ShowString(20, 210, 200, 16, 16, "*    KEY0: Zhou Zhun    *");
	LCD_ShowString(20, 230, 200, 16, 16, "*    KEY1: Xu Mengyan   *");
	LCD_ShowString(20, 250, 200, 16, 16, "*    KEY2: Fu Yiqing    *");
	LCD_ShowString(20, 270, 200, 16, 16, "*    KEYUP: Exit        *");
	LCD_ShowString(20, 290, 200, 16, 16, "=========================");
}

/**
 * @brief 添加用户ID
 */
void AddUserID(void)
{
	char status;
	u32 index = 0;
	// u8 name[18] = "周明发徐梦燕付衣情周准";
	keyFun = KEY_NULL; // 先初始化没有任何按键按下的状态
	while (1)
	{
		ShowMenu();
		ShowTime(DHM); // 显示时间

		/* KEY_UP 按下 则退出 */
		if (KEYUP_VALUE == keyFun)
		{
			PcdHalt();
			LCD_Clear(BLUE);
			return;
		}

		/* 存在按键按下 且 按下的按键不是KEYUP */
		while ((KEY_NULL != keyFun) && (keyFun != KEYUP_VALUE))
		{
			status = ReadCard(); // 读卡
			if (status == MI_OK)
			{
				index = SerachID(CardID); // 在数据存储区搜索用户卡ID

				if (index == FLASH_SIZE) // 数据存储区已满 则退出
				{
					LCD_ShowString(30, 80, 200, 16, 16, "FLASH Overflow!");
					ERR_BEEP();
					delay_ms(1000);
					LCD_Clear(BLUE);
					break;
				}

				/* 用户卡ID不存在数据存储中 则可以注册*/
				if (index != EXIST)
				{
					W25QXX_Write(CardID, index, sizeof(CardID)); // 写入到W25QXX数据存储区中
					// LCD_Clear(BLUE);
					LCD_ShowString(30, 80, 200, 16, 16, "OK: User Add Succeed!");
					OK_BEEP();
					delay_ms(1000);
					// LCD_Clear(BLUE);
					LCD_Clear(BLUE);
				}
				else
				{
					LCD_ShowString(20, 100, 200, 16, 16, "Fail: User Already exists.");
					ERR_BEEP();
					delay_ms(1000);
					LCD_Clear(BLUE);
				}

				keyFun = KEY_NULL; // 执行一次注册功能 将按键置空
			}
			else /* 读卡失败 重新读卡 */
			{
				LCD_ShowString(30, 100, 200, 16, 16, "Error: Read card again!");
				ERR_BEEP();
				delay_ms(1000);
				LCD_Clear(BLUE);
			}
		}
	}
}

void ReadCardTips(void)
{
	LCD_Clear(BLUE);
	LCD_ShowString(20, 150, 200, 16, 16, "=========================");
	LCD_ShowString(20, 170, 200, 16, 16, "*    ====(((())))====   *");
	LCD_ShowString(20, 190, 200, 16, 16, "*        Reading...     *");
	LCD_ShowString(20, 210, 200, 16, 16, "*       KEYUP:Exit      *");
	LCD_ShowString(20, 230, 200, 16, 16, "=========================");
}

/**
 * @brief RFID读卡
 * @return char 返回状态
 */
char ReadCard(void)
{
	char status;
	u8 Card_Type[4];
	ReadCardTips(); // 读卡提示
	while (1)
	{
		status = PcdRequest(0x52, Card_Type);
		status = PcdAnticoll(CardID); /*防冲撞*/
		status = PcdSelect(CardID);	  //选卡
		// status = PcdAuthState(PICC_AUTHENT1A, 1, defaultKey, CardID);
		if (status == MI_OK)
			break;
		if (keyFun == KEYUP_VALUE)
		{
			LCD_Clear(BLUE);
			return MI_ERR; // 可强制退出
		}
	}
	LCD_Clear(BLUE);
	return MI_OK;
}

/**
 * @brief DoorControlSystem(DCS)初始化
 */
void DCSInit(void)
{
	LCD_Clear(BLUE);					 // 清屏为蓝色 即背景颜色
	BACK_COLOR = BLUE;					 // 设置背景颜色为蓝色
	POINT_COLOR = RED;					 // 设置字体为红色
	LCD_DrawRectangle(20, 20, 240, 116); // 绘制一个矩形框
	LCD_ShowString(30, 30, 200, 16, 16, "DoorControlSystem(DCS)");
	LCD_ShowString(30, 50, 200, 16, 16, "   @Version: 1.2.1    ");
	LCD_ShowString(30, 70, 200, 16, 16, "     @Author: DXR     ");
	LCD_ShowString(30, 90, 200, 16, 16, "      2021/11/25      ");
}
