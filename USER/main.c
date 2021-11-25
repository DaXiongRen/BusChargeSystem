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

/** ���߷�ʽ
	PC10 -- CLK
	PC11 -- MISO
	PC12 -- MOSI
	PA4  -- NSS
	PA6  -- RST
*/

u8 CardID[ID_LEN];	  // ���ID����
u8 keyFun = KEY_NULL; // �ȳ�ʼ��û���κΰ������µ�״̬

/* ����FLASH�йر��� ��FLASH_START = FLASH_SIZEʱ ��Ϊ�洢�������� */
const u8 START_ADDR = 10;			  // FLASH�洢����ʼ��ַ
const u32 FLASH_SIZE = 16 * 1024 * 8; // FLASH��СΪ16KB

int main(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //����ϵͳ�ж����ȼ�����2
	delay_init(168);								//��ʼ����ʱ����
	LED_Init();										//��ʼ��LED
	BEEP_Init();									//��ʼ��������
	KEY_Init();										//��ʼ������
	EXTIX_Init();									// ��ʼ���ⲿ�ж� ���ڶ�ȡ����ֵ
	W25QXX_Init();									//��ʼ��W25Q128
	uart_init(115200);								//���ڳ�ʼ��������Ϊ115200
	LCD_Init();										//LCD��ʼ��
	RC522_SPI3_Init();
	MFRC522_Initializtion();

	POINT_COLOR = RED; // ��������Ϊ��ɫ
	/* ���W25Q128 */
	while (W25QXX_ReadID() != W25Q128)
	{
		LCD_ShowString(30, 90, 200, 16, 16, "W25Q128 Check Failed!");
		delay_ms(500);
		LCD_ShowString(30, 90, 200, 16, 16, "Please Check!      ");
		delay_ms(500);
		LED0 = !LED0; // DS0��˸
	}
	LCD_ShowString(30, 90, 200, 16, 16, "W25Q128 Ready!");
	LCD_ShowString(30, 110, 200, 16, 16, "System loading...");
	delay_ms(3000);
	DCSInit();	  // DoorControlSystem(DCS)��ʼ��
	DCSRunning(); // DoorControlSystem(DCS)����
}

/**
 * @brief DoorControlSystem(DCS)����
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
			UserLogin();	   // �û���¼ -> ������Ϣ
			keyFun = KEY_NULL; // ��ΪKEY_NULL -> ��ֹ�´�ѭ���������ô˹���
			break;
		case KEY1_VALUE:
			AddUserID();	   // ����û� ����û���Ҳ��Ҫ����
			keyFun = KEY_NULL; // ��ΪKEY_NULL -> ��ֹ�´�ѭ���������ô˹���
			break;
		case KEY2_VALUE:
			status = ReadCard(); // ���� �˰���ֻ��Ϊ�˲����Ƿ�����ȷ����
			if (status == MI_OK)
			{
				OK_BEEP();
			}
			keyFun = KEY_NULL; // ��ΪKEY_NULL -> ��ֹ�´�ѭ���������ô˹���
			break;
		}
	}
}

/**
 * @brief ��ʾLCD����
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
 * @brief �û���¼
 */
void UserLogin(void)
{
	u8 status;
	while (1)
	{
		status = ReadCard();
		if (status == MI_OK)
		{
			/* �û���ID���� �Ϸ��û� */
			if (SerachID(CardID) == EXIST)
			{
				LCD_ShowString(80, 110, 200, 16, 16, "Open door!");
				/* �����ɹ���ʾ */
				OK_BEEP();
				OK_LED();
			}
			else /* �û���ID������ �Ƿ��û� */
			{
				LCD_ShowString(80, 110, 200, 16, 16, "Error!");
				/* ����������ʾ */
				ERR_BEEP();
				ERR_LED();
			}
		}
		else /* ����ʧ�� ���¶��� */
		{
			LCD_Clear(BLUE);
			LCD_ShowString(80, 110, 200, 16, 16, "Read card again!");
			ERR_BEEP();
			delay_ms(1000);
		}
		/* �û���ǿ���˳�����ģʽ */
		if (keyFun == KEYUP_VALUE)
		{
			return;
		}
	}
}

/**
 * @brief �����ݴ洢�������û�����ID
 * @param CardID[u8*] �û���ID
 * @return u32 
 * ���û���ID���� �򷵻ط����û���ID�Ѵ��ڱ�־
 * ���û���ID������ �򷵻ص�ǰ�ɴ���ID�����ݴ洢���ĵ�ַ
 * �����ݴ洢������ ����FLASH_SIZE
 */
u32 SerachID(u8 *ID)
{
	u8 IDValue[ID_LEN];
	u8 i;
	for (i = START_ADDR; i < FLASH_SIZE; i += 4)
	{
		W25QXX_Read(IDValue, i, sizeof(ID));

		/* ���ݴ洢�����޴��û���ID */
		if (IDValue[0] == 0x0 && IDValue[1] == 0x0 && IDValue[2] == 0x0 && IDValue[3] == 0x0)
		{
			return i; // ���ص�ǰ�ɴ������ݴ洢���ĵ�ַ
		}
		/* �ж϶�ȡ����ID�Ƿ���ȷ */
		if (IsEquals(IDValue, ID) == TRUE)
		{
			return EXIST; // �����û���ID�Ѵ��ڱ�־
		}
	}
	return FLASH_SIZE; // ���ݴ洢������
}

/**
 * @brief �Ƚ����������Ƿ����
 * @param arr1[u8*]
 * @param arr2[u8*] 
 * @return Boolean ��ȷ���TRUE ���򷵻�FALSE
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
 * @brief ���ظ���ģʽ��ʱ�� �� ��ʾʱ��
 * @param timeMode[u8] ѡ���ģʽ
 * @return u8 D������ H����ʱ��M���ط�  DHM��ʾʱ�䣬�޷���
 */
u8 ShowTime(u8 timeMode)
{
	RTC_TimeTypeDef TheTime; //ʱ��
	RTC_DateTypeDef TheData; //����
	u8 *TimeBuff;			 //ʱ��洢

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
 * @brief ��ʾ����û��Ĳ˵�
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
 * @brief ����û�ID
 */
void AddUserID(void)
{
	char status;
	u32 index = 0;
	// u8 name[18] = "�����������ึ������׼";
	keyFun = KEY_NULL; // �ȳ�ʼ��û���κΰ������µ�״̬
	while (1)
	{
		ShowMenu();
		ShowTime(DHM); // ��ʾʱ��

		/* KEY_UP ���� ���˳� */
		if (KEYUP_VALUE == keyFun)
		{
			PcdHalt();
			LCD_Clear(BLUE);
			return;
		}

		/* ���ڰ������� �� ���µİ�������KEYUP */
		while ((KEY_NULL != keyFun) && (keyFun != KEYUP_VALUE))
		{
			status = ReadCard(); // ����
			if (status == MI_OK)
			{
				index = SerachID(CardID); // �����ݴ洢�������û���ID

				if (index == FLASH_SIZE) // ���ݴ洢������ ���˳�
				{
					LCD_ShowString(30, 80, 200, 16, 16, "FLASH Overflow!");
					ERR_BEEP();
					delay_ms(1000);
					LCD_Clear(BLUE);
					break;
				}

				/* �û���ID���������ݴ洢�� �����ע��*/
				if (index != EXIST)
				{
					W25QXX_Write(CardID, index, sizeof(CardID)); // д�뵽W25QXX���ݴ洢����
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

				keyFun = KEY_NULL; // ִ��һ��ע�Ṧ�� �������ÿ�
			}
			else /* ����ʧ�� ���¶��� */
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
 * @brief RFID����
 * @return char ����״̬
 */
char ReadCard(void)
{
	char status;
	u8 Card_Type[4];
	ReadCardTips(); // ������ʾ
	while (1)
	{
		status = PcdRequest(0x52, Card_Type);
		status = PcdAnticoll(CardID); /*����ײ*/
		status = PcdSelect(CardID);	  //ѡ��
		// status = PcdAuthState(PICC_AUTHENT1A, 1, defaultKey, CardID);
		if (status == MI_OK)
			break;
		if (keyFun == KEYUP_VALUE)
		{
			LCD_Clear(BLUE);
			return MI_ERR; // ��ǿ���˳�
		}
	}
	LCD_Clear(BLUE);
	return MI_OK;
}

/**
 * @brief DoorControlSystem(DCS)��ʼ��
 */
void DCSInit(void)
{
	LCD_Clear(BLUE);					 // ����Ϊ��ɫ ��������ɫ
	BACK_COLOR = BLUE;					 // ���ñ�����ɫΪ��ɫ
	POINT_COLOR = RED;					 // ��������Ϊ��ɫ
	LCD_DrawRectangle(20, 20, 240, 116); // ����һ�����ο�
	LCD_ShowString(30, 30, 200, 16, 16, "DoorControlSystem(DCS)");
	LCD_ShowString(30, 50, 200, 16, 16, "   @Version: 1.2.1    ");
	LCD_ShowString(30, 70, 200, 16, 16, "     @Author: DXR     ");
	LCD_ShowString(30, 90, 200, 16, 16, "      2021/11/25      ");
}
