/**
 * @file main.c
 * @author DXR (daxiongren@foxmail.com)
 * @brief 主程序源文件
 * @version 2.2
 * @date 2021-12-02
 */
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
	STM32 --  RFID
	PA4   --  SDA
	PC10  --  SCK
	PC12  --  MOSI
	PC11  --  MISO
	PA6   --  RST
*/
u8 adminID[ID_LEN] = {0x3A, 0x60, 0xAE, 0x80}; // 管理员卡号 用于删除用户的权限控制
u8 cardID[ID_LEN + 1];						   // 存放普通用户ID卡号 +1是为了存放'\0'
u8 userName[NAME_LEN + 1];					   // 用户名 +1是为了存放'\0'
u8 balance;									   // 账户余额
u8 keyFun;									   // 先初始化没有任何按键按下的状态
/**
 * 在Flash中存储数据的格式：
 * 【顺序存储】
 * 4个字节：用户卡ID
 * 3个字节：用户名
 * 1个字节：账户余额
 * 共占8字节
 */
/* 定义FLASH有关变量 当START_ADDR = MAX_ADDR时 则为存储容量已满 */
const u32 MAX_ADDR = 0x00FFFF;						  // 取W25Q128中的一个块内存 MAX_ADDR = 65535; W25Q128总内存是16M = 16 * 1024 * 1024 Byte
const u32 START_ADDR = 7;							  // 定义W25Q128一个块内存的起始地址
const u8 DATA_SIZE = ID_LEN + NAME_LEN + BALANCE_LEN; // 数据大小
/* Flash[START_ADDR,MAX_ADDR] */

int main(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //设置系统中断优先级分组2
	delay_init(168);								//初始化延时函数
	LED_Init();										//初始化LED
	BEEP_Init();									//初始化蜂鸣器
	KEY_Init();										//初始化按键
	EXTIX_Init();									// 初始化外部中断 用于读取按键值
	W25QXX_Init();									//初始化W25Q128
	uart_init(115200);								//串口1初始化波特率为115200
	LCD_Init();										// LCD初始化
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
	delay_ms(2000);
	BCSInit();	  // BusChargeSystem(BCS)初始化
	SysRunning(); // 运行入口
}

/**
 * @brief 运行入口
 */
void SysRunning(void)
{
	while (1)
	{
		LcdDesktop();
		switch (keyFun)
		{
		case KEY0_VALUE:
			LCD_Clear(BLUE);
			BCSRunning();	   // 系统运行
			keyFun = KEY_NULL; // 置为KEY_NULL -> 防止下次循环继续调用此功能
			break;
		case KEY1_VALUE:
			LCD_Clear(BLUE);
			ManageUser();	   // 管理用户
			keyFun = KEY_NULL; // 置为KEY_NULL -> 防止下次循环继续调用此功能
			break;
		case KEY2_VALUE:
			LCD_Clear(BLUE);
			// DeleteUser();
			BalanceRecharge(); // 余额充值
			keyFun = KEY_NULL;
			break;
		}
	}
}

/**
 * @brief 显示LCD桌面
 */
void LcdDesktop(void)
{
	// 系统信息
	LCD_DrawRectangle(20, 20, 220, 116); // 绘制一个外矩形框
	LCD_DrawRectangle(22, 22, 218, 114); // 绘制一个内矩形框
	LCD_ShowString(30, 30, 200, 16, 16, "BusChargeSystem(BCS)  ");
	LCD_ShowString(30, 50, 200, 16, 16, "   @Version: 2.2      ");
	LCD_ShowString(30, 70, 200, 16, 16, "   @Author: DXR       ");
	LCD_ShowString(30, 90, 200, 16, 16, "   @Date: 2021/12/2   ");
	// 桌面信息
	LCD_ShowString(20, 130, 200, 16, 16, "=========================");
	LCD_ShowString(20, 150, 200, 16, 16, "*       <Desktop>       *");
	LCD_ShowString(20, 170, 200, 16, 16, "=========================");
	LCD_ShowString(20, 190, 200, 16, 16, "*    KEY0 <Running>     *");
	LCD_ShowString(20, 210, 200, 16, 16, "*    KEY1 <Manage User> *");
	LCD_ShowString(20, 230, 200, 16, 16, "*    KEY2 <Recharge>    *");
	LCD_ShowString(20, 250, 200, 16, 16, "=========================");
}

void ConsumeTips()
{
	LCD_DrawRectangle(20, 130, 220, 230); // 绘制一个外矩形框
	LCD_DrawRectangle(22, 132, 218, 228); // 绘制一个内矩形框
	LCD_ShowString(30, 150, 200, 16, 16, "    ====(((())))====   ");
}

/**
 * @brief BusChargeSystem(BCS)运行
 */
void BCSRunning(void)
{
	u8 status;
	u8 tempCardID[ID_LEN] = {0x0};
	u32 index; // 用于保存用户卡ID在数据存储区中的地址索引
	u8 consume = 0;
	LCD_Clear(BLUE);
	while (1)
	{
		ConsumeTips();
		status = ReadCard();
		if (status == MI_OK)
		{
			/* 用户卡ID存在 合法用户 */
			if (SearchID(cardID, &index) == TRUE)
			{
				/* 如果上一次刷卡的ID和这一次刷卡的ID不相同 那么就认为有新的用户正在消费 */
				if (IsEquals(cardID, tempCardID, ID_LEN) == FALSE)
				{
					consume = 0; // 则把消费金额置零
				}
				balance = GetBalance(); // 获取当前用户的余额
				/* 余额必须大于0 才可以消费 */
				if (balance > 0)
				{
					consume++;
					LCD_ShowNum(75, 180, consume, 3, 24); // 显示消费金额
					LCD_ShowString(115, 180, 200, 16, 24, "RMB");
					SetBalance(--balance);				// 用户余额 - 1
					memcpy(tempCardID, cardID, ID_LEN); // 把这次消费的ID复制到临时存储区
					/* 读卡成功提示 */
					OK_BEEP();
					OK_LED(); // 有两秒的延时
					LCD_Clear(BLUE);
				}
				else /* 余额不足 */
				{
					LCD_ShowString(40, 180, 200, 16, 16, "Error:Balance is null!");
					/* 读卡错误提示 */
					ERR_BEEP();
					ERR_LED();
					LCD_Clear(BLUE);
				}
			}
			else /* 用户卡ID不存在 非法用户 */
			{
				memset(tempCardID, 0x0, ID_LEN);
				LCD_ShowString(40, 180, 200, 16, 16, "Error:Not fuond user!");
				/* 读卡错误提示 */
				ERR_BEEP();
				ERR_LED();
				LCD_Clear(BLUE);
			}
		}
		/* 用户可强制退出运行模式 */
		if (keyFun == KEYUP_VALUE)
		{
			LCD_Clear(BLUE);
			LCD_ShowString(40, 110, 200, 16, 16, "Tips: Exiting...");
			// ERR_BEEP();
			delay_ms(1000);
			LCD_Clear(BLUE);
			break;
		}
		// LCD_Clear(BLUE);
	}
}

/**
 * @brief 获取用户余额
 * @return u8 返回当前ID对应的用户余额
 */
u8 GetBalance()
{
	u32 index;
	u8 balance;
	/* 当前用户ID不存在则返回的余额为0 */
	if (SearchID(cardID, &index) == FALSE)
	{
		return 0;
	}
	/* 存在 则从内存中读取 */
	W25QXX_Read(&balance, index + ID_LEN + NAME_LEN, BALANCE_LEN);
	return balance;
}

/**
 * @brief 设置当前ID的用户余额
 * @param balance
 */
void SetBalance(u8 balance)
{
	u32 index;
	/* 当前用户ID不存在 则不能不能存入 直接返回 */
	if (SearchID(cardID, &index) == FALSE)
	{
		return;
	}
	W25QXX_Write(&balance, index + ID_LEN + NAME_LEN, BALANCE_LEN);
}

/**
 * @brief 显示充值菜单
 */
void RechargeMenu(void)
{
	LCD_ShowString(20, 130, 200, 16, 16, "=========================");
	LCD_ShowString(20, 150, 200, 16, 16, "*       <Recharge>      *");
	LCD_ShowString(20, 170, 200, 16, 16, "=========================");
	LCD_ShowString(20, 190, 200, 16, 16, "*      KEY1: 100 RMB    *");
	LCD_ShowString(20, 210, 200, 16, 16, "*      KEY2: 200 RMB    *");
	LCD_ShowString(20, 230, 200, 16, 16, "*      KEYUP: Exit      *");
	LCD_ShowString(20, 250, 200, 16, 16, "=========================");
}

/**
 * @brief 余额充值
 */
void BalanceRecharge(void)
{
	u32 index;
	u8 status;
	keyFun = KEY_NULL; // 先初始化没有任何按键按下的状态
	LCD_Clear(BLUE);
	while (1)
	{
		RechargeMenu(); // 显示充值菜单
		/* KEY_UP 按下 则退出 */
		if (KEYUP_VALUE == keyFun)
		{
			LCD_Clear(BLUE);
			return;
		}

		if (keyFun == KEY1_VALUE || keyFun == KEY2_VALUE)
		{
			LCD_Clear(BLUE);
			ReadCardTips();		 // 提示读卡信息
			status = ReadCard(); // 读卡
			if (status == MI_OK)
			{
				if (SearchID(cardID, &index) == TRUE)
				{
					balance = GetBalance();			  // 先获取用户的当前余额
					if (keyFun * 100 + balance > 200) // 验证是否超出限额
					{
						LCD_Clear(BLUE);
						LCD_ShowString(30, 110, 220, 16, 16, "Balance Exceeded!");
						ERR_BEEP();
						delay_ms(1000);
						LCD_Clear(BLUE);
						keyFun = KEY_NULL;
						continue;
					}
					SetBalance(keyFun * 100 + balance);
					LCD_Clear(BLUE);
					balance = GetBalance();
					OK_BEEP();
					LCD_ShowString(30, 110, 220, 16, 16, "OK:Recharge succeed!");
					LCD_ShowNum(75, 180, balance, 3, 24);
					LCD_ShowString(115, 180, 200, 16, 24, "RMB");
					delay_ms(3000);
					LCD_Clear(BLUE);
					keyFun = KEY_NULL;
				}
				else
				{
					LCD_Clear(BLUE);
					LCD_ShowString(30, 110, 220, 16, 16, "Error:Not fuond user! ");
					ERR_BEEP();
					delay_ms(1000);
					LCD_Clear(BLUE);
					keyFun = KEY_NULL;
				}
			}
			else
			{
				keyFun = KEY_NULL; // 退出
			}
		}
	}
}

/**
 * @brief 在数据存储区搜索用户卡的ID
 * @param cardID[u8*](IN) 用户卡ID
 * @param index[u32*](OUT) 指向数据存储区地址的索引
 * @return Boolean
 * 若用户卡ID存在 则返回该ID在存储区中的地址 并返回TRUE
 * 若用户卡ID不存在 则返回数据存储区中首个空数据的地址 并返回FALSE
 * 若遍历了整个地址段都无此用户 则返回MAX_ADDR + 1 并返回FALSE
 */
Boolean SearchID(u8 *cardID, u32 *index)
{
	u8 IDValue[ID_LEN];
	u32 i;
	for (i = START_ADDR; i <= MAX_ADDR; i += DATA_SIZE)
	{
		W25QXX_Read(IDValue, i, ID_LEN);
		/* 数据存储区中无此用户卡ID */
		if (IsNullID(IDValue, ID_LEN) == TRUE)
		{
			*index = i; // 返回当前可存入数据存储区的地址 即空数据地址
			return FALSE;
		}
		/* 判断读取到的ID是否正确 */
		if (IsEquals(IDValue, cardID, ID_LEN) == TRUE)
		{
			*index = i; // 返回该ID在存储区中的地址
			return TRUE;
		}
	}
	*index = MAX_ADDR + 1; // 返回MAX_ADDR + 1
	return FALSE;		   // 遍历了整个地址段都无此用户 则返回FALSE
}

/**
 * @brief 比较两个数组是否相等
 * @param arr1[u8*]
 * @param arr2[u8*]
 * @return Boolean 相等返回TRUE 否则返回FALSE
 */
Boolean IsEquals(u8 *arr1, u8 *arr2, u8 len)
{
	u8 i;
	u8 ans = 0;
	/* 通过异或运算校验每一位 */
	for (i = 0; i < len; i++)
	{
		ans |= arr1[i] ^ arr2[i];
	}
	/* 如果最终ans还是0 则说明两个数组相等返回TRUE 否则不相等返回FALSE */
	return ans == 0 ? TRUE : FALSE;
}

/**
 * @brief 判断ID是否为空
 * @param cardID
 * @param size
 * @return Boolean
 */
Boolean IsNullID(u8 *cardID, u8 size)
{
	u8 *nullID = (u8 *)malloc(size);
	Boolean check = FALSE;
	memset(nullID, 0xFF, size);
	check = IsEquals(cardID, nullID, size);
	free(nullID); // 释放内存
	return check;
}

/**
 * @brief 判断数据是否为空
 * @param data
 * @param size
 * @return Boolean
 */
Boolean IsNullData(u8 *data, u8 size)
{
	u8 *nullData = (u8 *)malloc(size);
	Boolean check = FALSE;
	memset(nullData, 0xFF, size);
	check = IsEquals(data, nullData, size);
	free(nullData); // 释放内存
	return check;
}

/**
 * @brief 读卡提示信息
 */
void ReadCardTips(void)
{
	// LCD_Clear(BLUE);
	LCD_ShowString(20, 130, 200, 16, 16, "=========================");
	LCD_ShowString(20, 150, 200, 16, 16, "*    ====(((())))====   *");
	LCD_ShowString(20, 170, 200, 16, 16, "*        Reading...     *");
	LCD_ShowString(20, 190, 200, 16, 16, "*       KEYUP:Exit      *");
	LCD_ShowString(20, 210, 200, 16, 16, "=========================");
}

/**
 * @brief RFID读卡
 * @return char 返回状态
 */
char ReadCard(void)
{
	char status;
	u8 cardType[ID_LEN];
	// ReadCardTips(); // 读卡提示信息
	while (1)
	{
		status = PcdRequest(0x52, cardType); // 复位应答
		status = PcdAnticoll(cardID);		 /*防冲撞*/
		status = PcdSelect(cardID);			 //选卡
		if (status == MI_OK)
		{
			break;
		}
		/* 可强制退出 */
		if (keyFun == KEYUP_VALUE)
		{
			// LCD_Clear(BLUE);
			return MI_ERR;
		}
	}
	// LCD_Clear(BLUE);
	return MI_OK;
}

/**
 * @brief 管理用户菜单
 */
void ManageUserMenu(void)
{
	LCD_ShowString(20, 130, 200, 16, 16, "=========================");
	LCD_ShowString(20, 150, 200, 16, 16, "*      <Manage User>    *");
	LCD_ShowString(20, 170, 200, 16, 16, "=========================");
	LCD_ShowString(20, 190, 200, 16, 16, "*    KEY0: Add User     *");
	LCD_ShowString(20, 210, 200, 16, 16, "*    KEY1: Delete User  *");
	LCD_ShowString(20, 230, 200, 16, 16, "*    KEYUP: Exit        *");
	LCD_ShowString(20, 250, 200, 16, 16, "=========================");
}

/**
 * @brief 管理用户
 */
void ManageUser(void)
{
	u8 status;

	keyFun = KEY_NULL;

	while (1)
	{
		ManageUserMenu();
		/* KEY_UP 按下 则退出 */
		if (KEYUP_VALUE == keyFun)
		{
			// PcdHalt();
			LCD_Clear(BLUE);
			return;
		}
		/* 添加用户 -> 用户注册 */
		if (keyFun == KEY0_VALUE)
		{
			UserSignup();
			keyFun = KEY_NULL;
		}
		/* 删除用户 */
		if (keyFun == KEY1_VALUE)
		{
			LCD_Clear(BLUE);
			LCD_ShowString(20, 110, 200, 16, 16, "*      <Admin Login>    *");
			ReadCardTips();
			status = ReadCard(); // 读卡
			if (status == MI_OK)
			{
				/* 验证管理员 */
				if (IsEquals(cardID, adminID, ID_LEN) == TRUE)
				{
					OK_BEEP();
					DeleteUser();
					keyFun = KEY_NULL;
				}
				else
				{
					LCD_Clear(BLUE);
					LCD_ShowString(30, 110, 220, 16, 16, "Error:Non-Administrator!");
					ERR_BEEP();
					delay_ms(1000);
					LCD_Clear(BLUE);
					keyFun = KEY_NULL;
				}
			}
			else
			{
				LCD_Clear(BLUE);
				keyFun = KEY_NULL;
			}
		}
	}
}

/**
 * @brief 显示添加用户菜单
 */
void AddUserMenu(void)
{
	LCD_ShowString(20, 130, 200, 16, 16, "=========================");
	LCD_ShowString(20, 150, 200, 16, 16, "*       <Add User>      *");
	LCD_ShowString(20, 170, 200, 16, 16, "=========================");
	LCD_ShowString(20, 190, 200, 16, 16, "*    KEY0: Zhou Mingfa  *");
	LCD_ShowString(20, 210, 200, 16, 16, "*    KEY1: Xu Mengyan   *");
	LCD_ShowString(20, 230, 200, 16, 16, "*    KEY2: Fu Yiqing    *");
	LCD_ShowString(20, 250, 200, 16, 16, "*    KEYUP: Exit        *");
	LCD_ShowString(20, 270, 200, 16, 16, "=========================");
}

/**
 * @brief 注册
 */
void UserSignup(void)
{
	char status;
	u8 name[10] = "ZMFXMYFYQ";
	keyFun = KEY_NULL; // 先初始化没有任何按键按下的状态
	while (1)
	{
		AddUserMenu();
		ShowTime(DHM); // 显示时间

		/* KEY_UP 按下 则退出 */
		if (KEYUP_VALUE == keyFun)
		{
			// PcdHalt();
			LCD_Clear(BLUE);
			return;
		}

		/* 存在按键按下 且 按下的按键不是KEYUP */
		if ((KEY_NULL != keyFun) && (keyFun != KEYUP_VALUE))
		{
			LCD_Clear(BLUE);
			ReadCardTips();
			status = ReadCard(); // 读卡
			if (status == MI_OK)
			{
				/* 存入用户名 且 注册默认充值10 */
				memcpy(userName, &name[keyFun * NAME_LEN], NAME_LEN);
				balance = 10;
				AddUser(cardID);
				keyFun = KEY_NULL; // 执行一次注册功能 将按键置空
			}
			else
			{
				keyFun = KEY_NULL;
			}
		}
	}
}

/**
 * @brief 通过传入的用户卡ID添加用户
 * @param cardID
 */
void AddUser(u8 *cardID)
{
	/* 添加成功 */
	if (AddData(cardID) == TRUE)
	{
		LCD_Clear(BLUE);
		LCD_ShowString(20, 110, 200, 16, 16, "OK:Signup succeed!");
		LCD_ShowString(20, 140, 200, 16, 16, "Name:");
		LCD_ShowString(80, 135, 200, 16, 24, userName); // 显示用户名
		LCD_ShowString(20, 170, 200, 16, 16, "Balance:");
		LCD_ShowNum(80, 165, balance, 3, 24); // 显示余额
		LCD_ShowString(120, 165, 200, 16, 24, "RMB");

		OK_BEEP();
		delay_ms(3000);
		LCD_Clear(BLUE);
	}
	else /* 添加失败 */
	{
		LCD_Clear(BLUE);
		LCD_ShowString(20, 110, 200, 16, 16, "Error:Signup fail!");
		ERR_BEEP();
		delay_ms(1000);
		LCD_Clear(BLUE);
	}
}

/**
 * @brief 添加用户数据
 * @param cardID[u8*]
 * @return Boolean
 */
Boolean AddData(u8 *cardID)
{
	Boolean check; // 用户保存用户卡ID是否在数据存储区中的结果
	u32 index;	   // 用于保存用户卡ID在数据存储区中的地址索引

	check = SearchID(cardID, &index); // 在数据存储区搜索用户卡ID
	/* 防止达到数据存储区最大地址时添加 */
	if (index > MAX_ADDR)
	{
		LCD_Clear(BLUE);
		LCD_ShowString(20, 110, 200, 16, 16, "Error:Memory already full!");
		delay_ms(1000);
		LCD_Clear(BLUE);
		return FALSE;
	}
	/* 用户卡已存在 添加失败 */
	if (check == TRUE)
	{
		LCD_Clear(BLUE);
		LCD_ShowString(20, 110, 200, 16, 16, "Error:User already exist!");
		delay_ms(1000);
		LCD_Clear(BLUE);
		return FALSE;
	}
	/* 用户卡ID不存在数据存储中 */
	/* 写入到W25QXX数据存储区中 */
	W25QXX_Write(cardID, index, ID_LEN);
	/* 移动一个ID字节长度的地址继续写入用户名 */
	W25QXX_Write(userName, index + ID_LEN, NAME_LEN);
	/* 移动一个ID字节+用户名字节长度的地址继续写入余额 */
	W25QXX_Write(&balance, index + ID_LEN + NAME_LEN, BALANCE_LEN);
	return TRUE;
}

/**
 * @brief 显示删除用户菜单
 *
 */
void DelUserMenu(void)
{
	LCD_ShowString(20, 130, 200, 16, 16, "=========================");
	LCD_ShowString(20, 150, 200, 16, 16, "*    <Please Select>    *");
	LCD_ShowString(20, 170, 200, 16, 16, "=========================");
	LCD_ShowString(20, 190, 200, 16, 16, "* KEY1: Delete a user   *");
	LCD_ShowString(20, 210, 200, 16, 16, "* KEY2: Delete all user *");
	LCD_ShowString(20, 230, 200, 16, 16, "* KEYUP: Exit           *");
	LCD_ShowString(20, 250, 200, 16, 16, "=========================");
}

/**
 * @brief 删除用户
 */
void DeleteUser(void)
{
	char status;
	keyFun = KEY_NULL;
	LCD_Clear(BLUE);
	while (1)
	{
		DelUserMenu();
		if (KEYUP_VALUE == keyFun)
		{
			LCD_Clear(BLUE);
			return;
		}

		/* 按KEY1删除指定用户卡ID 需刷指定的卡进行删除操作 */
		while (keyFun == KEY1_VALUE)
		{
			LCD_Clear(BLUE);
			ReadCardTips();
			status = ReadCard(); // 刷卡
			if (status == MI_OK)
			{
				RemoveUser(cardID);
				keyFun = KEY_NULL;
			}
			else /* 读卡失败 */
			{
				keyFun = KEY_NULL; // 强制退出
			}
		}

		/* 删除所有用户卡ID */
		if (keyFun == KEY2_VALUE)
		{
			RemoveAllUser();
			keyFun = KEY_NULL;
		}
	}
}

/**
 * @brief 删除所用用户
 */
void RemoveAllUser()
{
	keyFun = KEY_NULL;
	LCD_Clear(BLUE);
	/* 需确认是否进行删除所有用户卡ID的操作 */
	while (1)
	{
		LCD_ShowString(10, 110, 220, 16, 16, "Tips:Are you sure you want");
		LCD_ShowString(10, 130, 220, 16, 16, "     to continue?");
		LCD_ShowString(10, 150, 220, 16, 16, "Yes:KEY1          No:KEY2");
		if (keyFun == KEY1_VALUE)
		{
			LCD_Clear(BLUE);
			LCD_ShowString(10, 110, 220, 16, 16, "Tips:Deleting all users...");
			RemoveAllData(START_ADDR);
			LCD_ShowString(10, 110, 220, 16, 16, "OK:Delete all users succeed!");
			OK_BEEP();
			delay_ms(1000);
			break;
		}
		if (keyFun == KEY2_VALUE)
		{
			break;
		}
	}
	LCD_Clear(BLUE);
}

/**
 * @brief 删除所有用户数据
 * @param startAddr[u32] 数据存储区的起始地址
 */
void RemoveAllData(u32 startAddr)
{
	u32 i;
	u8 IDValue[DATA_SIZE];
	u8 nullData[DATA_SIZE];
	memset(nullData, 0xFF, DATA_SIZE); // 空ID值
	for (i = startAddr; i <= MAX_ADDR; i += DATA_SIZE)
	{
		W25QXX_Read(IDValue, i, DATA_SIZE);
		/* 结束条件 */
		if (IsEquals(IDValue, nullData, DATA_SIZE) == TRUE)
		{
			return; // 当读到空ID时 则为所用用户数据删除完成
		}
		W25QXX_Write(nullData, i, DATA_SIZE);
	}
}

/**
 * @brief 通过传入的用户卡ID删除指定用户数据
 * @param cardID
 */
void RemoveUser(u8 *cardID)
{
	/* 删除成功 */
	if (RemoveData(cardID) == TRUE)
	{
		LCD_Clear(BLUE);
		LCD_ShowString(10, 110, 220, 16, 16, "OK:Delete user succeed!");
		OK_BEEP();
		delay_ms(1000);
		LCD_Clear(BLUE);
	}
	else /* 无此用户 删除失败 */
	{
		LCD_Clear(BLUE);
		LCD_ShowString(10, 110, 220, 16, 16, "Error:Not fuond user! ");
		ERR_BEEP();
		delay_ms(1000);
		LCD_Clear(BLUE);
	}
}

/**
 * @brief 删除指定用户数据
 * @param cardID[u8*]
 * @return Boolean
 */
Boolean RemoveData(u8 *cardID)
{
	u32 i, index;
	u8 userData[DATA_SIZE]; // 用于存储 ID 用户名 账户余额
	Boolean check;
	check = SearchID(cardID, &index); // 搜索此用户卡ID
	/* 数据存储区存在此用户卡ID -> 删除 */
	if (check == TRUE)
	{
		/* 将后面的用户数据往前移动覆盖 即可将此用户数据 */
		for (i = index; i <= MAX_ADDR; i += DATA_SIZE)
		{
			/* Flash[i] = Flash[i + DATA_SIZE] */
			W25QXX_Read(userData, i + DATA_SIZE, DATA_SIZE);
			W25QXX_Write(userData, i, DATA_SIZE);
			/* 结束条件 */
			if (IsNullData(userData, DATA_SIZE) == TRUE)
			{
				return TRUE; // 移动结束 删除成功 返回TRUE
			}
		}
	}
	/* 用户不存在 删除失败 则返回FALSE */
	return FALSE;
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
 * @brief BusChargeSystem(BCS)初始化
 */
void BCSInit(void)
{
	LCD_Clear(BLUE);   // 清屏为蓝色 即背景颜色
	BACK_COLOR = BLUE; // 设置背景颜色为蓝色
	POINT_COLOR = RED; // 设置字体为红色
	keyFun = KEY_NULL; // 先初始化没有任何按键按下的状态
}
