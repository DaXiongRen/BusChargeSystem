/**
 * @file main.c
 * @author DXR (daxiongren@foxmail.com)
 * @brief ������Դ�ļ�
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

/** ���߷�ʽ
	STM32 --  RFID
	PA4   --  SDA
	PC10  --  SCK
	PC12  --  MOSI
	PC11  --  MISO
	PA6   --  RST
*/
u8 adminID[ID_LEN] = {0x3A, 0x60, 0xAE, 0x80}; // ����Ա���� ����ɾ���û���Ȩ�޿���
u8 cardID[ID_LEN + 1];						   // �����ͨ�û�ID���� +1��Ϊ�˴��'\0'
u8 userName[NAME_LEN + 1];					   // �û��� +1��Ϊ�˴��'\0'
u8 balance;									   // �˻����
u8 keyFun;									   // �ȳ�ʼ��û���κΰ������µ�״̬
/**
 * ��Flash�д洢���ݵĸ�ʽ��
 * ��˳��洢��
 * 4���ֽڣ��û���ID
 * 3���ֽڣ��û���
 * 1���ֽڣ��˻����
 * ��ռ8�ֽ�
 */
/* ����FLASH�йر��� ��START_ADDR = MAX_ADDRʱ ��Ϊ�洢�������� */
const u32 MAX_ADDR = 0x00FFFF;						  // ȡW25Q128�е�һ�����ڴ� MAX_ADDR = 65535; W25Q128���ڴ���16M = 16 * 1024 * 1024 Byte
const u32 START_ADDR = 7;							  // ����W25Q128һ�����ڴ����ʼ��ַ
const u8 DATA_SIZE = ID_LEN + NAME_LEN + BALANCE_LEN; // ���ݴ�С
/* Flash[START_ADDR,MAX_ADDR] */

int main(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //����ϵͳ�ж����ȼ�����2
	delay_init(168);								//��ʼ����ʱ����
	LED_Init();										//��ʼ��LED
	BEEP_Init();									//��ʼ��������
	KEY_Init();										//��ʼ������
	EXTIX_Init();									// ��ʼ���ⲿ�ж� ���ڶ�ȡ����ֵ
	W25QXX_Init();									//��ʼ��W25Q128
	uart_init(115200);								//����1��ʼ��������Ϊ115200
	LCD_Init();										// LCD��ʼ��
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
	delay_ms(2000);
	BCSInit();	  // BusChargeSystem(BCS)��ʼ��
	SysRunning(); // �������
}

/**
 * @brief �������
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
			BCSRunning();	   // ϵͳ����
			keyFun = KEY_NULL; // ��ΪKEY_NULL -> ��ֹ�´�ѭ���������ô˹���
			break;
		case KEY1_VALUE:
			LCD_Clear(BLUE);
			ManageUser();	   // �����û�
			keyFun = KEY_NULL; // ��ΪKEY_NULL -> ��ֹ�´�ѭ���������ô˹���
			break;
		case KEY2_VALUE:
			LCD_Clear(BLUE);
			// DeleteUser();
			BalanceRecharge(); // ����ֵ
			keyFun = KEY_NULL;
			break;
		}
	}
}

/**
 * @brief ��ʾLCD����
 */
void LcdDesktop(void)
{
	// ϵͳ��Ϣ
	LCD_DrawRectangle(20, 20, 220, 116); // ����һ������ο�
	LCD_DrawRectangle(22, 22, 218, 114); // ����һ���ھ��ο�
	LCD_ShowString(30, 30, 200, 16, 16, "BusChargeSystem(BCS)  ");
	LCD_ShowString(30, 50, 200, 16, 16, "   @Version: 2.2      ");
	LCD_ShowString(30, 70, 200, 16, 16, "   @Author: DXR       ");
	LCD_ShowString(30, 90, 200, 16, 16, "   @Date: 2021/12/2   ");
	// ������Ϣ
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
	LCD_DrawRectangle(20, 130, 220, 230); // ����һ������ο�
	LCD_DrawRectangle(22, 132, 218, 228); // ����һ���ھ��ο�
	LCD_ShowString(30, 150, 200, 16, 16, "    ====(((())))====   ");
}

/**
 * @brief BusChargeSystem(BCS)����
 */
void BCSRunning(void)
{
	u8 status;
	u8 tempCardID[ID_LEN] = {0x0};
	u32 index; // ���ڱ����û���ID�����ݴ洢���еĵ�ַ����
	u8 consume = 0;
	LCD_Clear(BLUE);
	while (1)
	{
		ConsumeTips();
		status = ReadCard();
		if (status == MI_OK)
		{
			/* �û���ID���� �Ϸ��û� */
			if (SearchID(cardID, &index) == TRUE)
			{
				/* �����һ��ˢ����ID����һ��ˢ����ID����ͬ ��ô����Ϊ���µ��û��������� */
				if (IsEquals(cardID, tempCardID, ID_LEN) == FALSE)
				{
					consume = 0; // ������ѽ������
				}
				balance = GetBalance(); // ��ȡ��ǰ�û������
				/* ���������0 �ſ������� */
				if (balance > 0)
				{
					consume++;
					LCD_ShowNum(75, 180, consume, 3, 24); // ��ʾ���ѽ��
					LCD_ShowString(115, 180, 200, 16, 24, "RMB");
					SetBalance(--balance);				// �û���� - 1
					memcpy(tempCardID, cardID, ID_LEN); // ��������ѵ�ID���Ƶ���ʱ�洢��
					/* �����ɹ���ʾ */
					OK_BEEP();
					OK_LED(); // ���������ʱ
					LCD_Clear(BLUE);
				}
				else /* ���� */
				{
					LCD_ShowString(40, 180, 200, 16, 16, "Error:Balance is null!");
					/* ����������ʾ */
					ERR_BEEP();
					ERR_LED();
					LCD_Clear(BLUE);
				}
			}
			else /* �û���ID������ �Ƿ��û� */
			{
				memset(tempCardID, 0x0, ID_LEN);
				LCD_ShowString(40, 180, 200, 16, 16, "Error:Not fuond user!");
				/* ����������ʾ */
				ERR_BEEP();
				ERR_LED();
				LCD_Clear(BLUE);
			}
		}
		/* �û���ǿ���˳�����ģʽ */
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
 * @brief ��ȡ�û����
 * @return u8 ���ص�ǰID��Ӧ���û����
 */
u8 GetBalance()
{
	u32 index;
	u8 balance;
	/* ��ǰ�û�ID�������򷵻ص����Ϊ0 */
	if (SearchID(cardID, &index) == FALSE)
	{
		return 0;
	}
	/* ���� ����ڴ��ж�ȡ */
	W25QXX_Read(&balance, index + ID_LEN + NAME_LEN, BALANCE_LEN);
	return balance;
}

/**
 * @brief ���õ�ǰID���û����
 * @param balance
 */
void SetBalance(u8 balance)
{
	u32 index;
	/* ��ǰ�û�ID������ ���ܲ��ܴ��� ֱ�ӷ��� */
	if (SearchID(cardID, &index) == FALSE)
	{
		return;
	}
	W25QXX_Write(&balance, index + ID_LEN + NAME_LEN, BALANCE_LEN);
}

/**
 * @brief ��ʾ��ֵ�˵�
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
 * @brief ����ֵ
 */
void BalanceRecharge(void)
{
	u32 index;
	u8 status;
	keyFun = KEY_NULL; // �ȳ�ʼ��û���κΰ������µ�״̬
	LCD_Clear(BLUE);
	while (1)
	{
		RechargeMenu(); // ��ʾ��ֵ�˵�
		/* KEY_UP ���� ���˳� */
		if (KEYUP_VALUE == keyFun)
		{
			LCD_Clear(BLUE);
			return;
		}

		if (keyFun == KEY1_VALUE || keyFun == KEY2_VALUE)
		{
			LCD_Clear(BLUE);
			ReadCardTips();		 // ��ʾ������Ϣ
			status = ReadCard(); // ����
			if (status == MI_OK)
			{
				if (SearchID(cardID, &index) == TRUE)
				{
					balance = GetBalance();			  // �Ȼ�ȡ�û��ĵ�ǰ���
					if (keyFun * 100 + balance > 200) // ��֤�Ƿ񳬳��޶�
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
				keyFun = KEY_NULL; // �˳�
			}
		}
	}
}

/**
 * @brief �����ݴ洢�������û�����ID
 * @param cardID[u8*](IN) �û���ID
 * @param index[u32*](OUT) ָ�����ݴ洢����ַ������
 * @return Boolean
 * ���û���ID���� �򷵻ظ�ID�ڴ洢���еĵ�ַ ������TRUE
 * ���û���ID������ �򷵻����ݴ洢�����׸������ݵĵ�ַ ������FALSE
 * ��������������ַ�ζ��޴��û� �򷵻�MAX_ADDR + 1 ������FALSE
 */
Boolean SearchID(u8 *cardID, u32 *index)
{
	u8 IDValue[ID_LEN];
	u32 i;
	for (i = START_ADDR; i <= MAX_ADDR; i += DATA_SIZE)
	{
		W25QXX_Read(IDValue, i, ID_LEN);
		/* ���ݴ洢�����޴��û���ID */
		if (IsNullID(IDValue, ID_LEN) == TRUE)
		{
			*index = i; // ���ص�ǰ�ɴ������ݴ洢���ĵ�ַ �������ݵ�ַ
			return FALSE;
		}
		/* �ж϶�ȡ����ID�Ƿ���ȷ */
		if (IsEquals(IDValue, cardID, ID_LEN) == TRUE)
		{
			*index = i; // ���ظ�ID�ڴ洢���еĵ�ַ
			return TRUE;
		}
	}
	*index = MAX_ADDR + 1; // ����MAX_ADDR + 1
	return FALSE;		   // ������������ַ�ζ��޴��û� �򷵻�FALSE
}

/**
 * @brief �Ƚ����������Ƿ����
 * @param arr1[u8*]
 * @param arr2[u8*]
 * @return Boolean ��ȷ���TRUE ���򷵻�FALSE
 */
Boolean IsEquals(u8 *arr1, u8 *arr2, u8 len)
{
	u8 i;
	u8 ans = 0;
	/* ͨ���������У��ÿһλ */
	for (i = 0; i < len; i++)
	{
		ans |= arr1[i] ^ arr2[i];
	}
	/* �������ans����0 ��˵������������ȷ���TRUE ������ȷ���FALSE */
	return ans == 0 ? TRUE : FALSE;
}

/**
 * @brief �ж�ID�Ƿ�Ϊ��
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
	free(nullID); // �ͷ��ڴ�
	return check;
}

/**
 * @brief �ж������Ƿ�Ϊ��
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
	free(nullData); // �ͷ��ڴ�
	return check;
}

/**
 * @brief ������ʾ��Ϣ
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
 * @brief RFID����
 * @return char ����״̬
 */
char ReadCard(void)
{
	char status;
	u8 cardType[ID_LEN];
	// ReadCardTips(); // ������ʾ��Ϣ
	while (1)
	{
		status = PcdRequest(0x52, cardType); // ��λӦ��
		status = PcdAnticoll(cardID);		 /*����ײ*/
		status = PcdSelect(cardID);			 //ѡ��
		if (status == MI_OK)
		{
			break;
		}
		/* ��ǿ���˳� */
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
 * @brief �����û��˵�
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
 * @brief �����û�
 */
void ManageUser(void)
{
	u8 status;

	keyFun = KEY_NULL;

	while (1)
	{
		ManageUserMenu();
		/* KEY_UP ���� ���˳� */
		if (KEYUP_VALUE == keyFun)
		{
			// PcdHalt();
			LCD_Clear(BLUE);
			return;
		}
		/* ����û� -> �û�ע�� */
		if (keyFun == KEY0_VALUE)
		{
			UserSignup();
			keyFun = KEY_NULL;
		}
		/* ɾ���û� */
		if (keyFun == KEY1_VALUE)
		{
			LCD_Clear(BLUE);
			LCD_ShowString(20, 110, 200, 16, 16, "*      <Admin Login>    *");
			ReadCardTips();
			status = ReadCard(); // ����
			if (status == MI_OK)
			{
				/* ��֤����Ա */
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
 * @brief ��ʾ����û��˵�
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
 * @brief ע��
 */
void UserSignup(void)
{
	char status;
	u8 name[10] = "ZMFXMYFYQ";
	keyFun = KEY_NULL; // �ȳ�ʼ��û���κΰ������µ�״̬
	while (1)
	{
		AddUserMenu();
		ShowTime(DHM); // ��ʾʱ��

		/* KEY_UP ���� ���˳� */
		if (KEYUP_VALUE == keyFun)
		{
			// PcdHalt();
			LCD_Clear(BLUE);
			return;
		}

		/* ���ڰ������� �� ���µİ�������KEYUP */
		if ((KEY_NULL != keyFun) && (keyFun != KEYUP_VALUE))
		{
			LCD_Clear(BLUE);
			ReadCardTips();
			status = ReadCard(); // ����
			if (status == MI_OK)
			{
				/* �����û��� �� ע��Ĭ�ϳ�ֵ10 */
				memcpy(userName, &name[keyFun * NAME_LEN], NAME_LEN);
				balance = 10;
				AddUser(cardID);
				keyFun = KEY_NULL; // ִ��һ��ע�Ṧ�� �������ÿ�
			}
			else
			{
				keyFun = KEY_NULL;
			}
		}
	}
}

/**
 * @brief ͨ��������û���ID����û�
 * @param cardID
 */
void AddUser(u8 *cardID)
{
	/* ��ӳɹ� */
	if (AddData(cardID) == TRUE)
	{
		LCD_Clear(BLUE);
		LCD_ShowString(20, 110, 200, 16, 16, "OK:Signup succeed!");
		LCD_ShowString(20, 140, 200, 16, 16, "Name:");
		LCD_ShowString(80, 135, 200, 16, 24, userName); // ��ʾ�û���
		LCD_ShowString(20, 170, 200, 16, 16, "Balance:");
		LCD_ShowNum(80, 165, balance, 3, 24); // ��ʾ���
		LCD_ShowString(120, 165, 200, 16, 24, "RMB");

		OK_BEEP();
		delay_ms(3000);
		LCD_Clear(BLUE);
	}
	else /* ���ʧ�� */
	{
		LCD_Clear(BLUE);
		LCD_ShowString(20, 110, 200, 16, 16, "Error:Signup fail!");
		ERR_BEEP();
		delay_ms(1000);
		LCD_Clear(BLUE);
	}
}

/**
 * @brief ����û�����
 * @param cardID[u8*]
 * @return Boolean
 */
Boolean AddData(u8 *cardID)
{
	Boolean check; // �û������û���ID�Ƿ������ݴ洢���еĽ��
	u32 index;	   // ���ڱ����û���ID�����ݴ洢���еĵ�ַ����

	check = SearchID(cardID, &index); // �����ݴ洢�������û���ID
	/* ��ֹ�ﵽ���ݴ洢������ַʱ��� */
	if (index > MAX_ADDR)
	{
		LCD_Clear(BLUE);
		LCD_ShowString(20, 110, 200, 16, 16, "Error:Memory already full!");
		delay_ms(1000);
		LCD_Clear(BLUE);
		return FALSE;
	}
	/* �û����Ѵ��� ���ʧ�� */
	if (check == TRUE)
	{
		LCD_Clear(BLUE);
		LCD_ShowString(20, 110, 200, 16, 16, "Error:User already exist!");
		delay_ms(1000);
		LCD_Clear(BLUE);
		return FALSE;
	}
	/* �û���ID���������ݴ洢�� */
	/* д�뵽W25QXX���ݴ洢���� */
	W25QXX_Write(cardID, index, ID_LEN);
	/* �ƶ�һ��ID�ֽڳ��ȵĵ�ַ����д���û��� */
	W25QXX_Write(userName, index + ID_LEN, NAME_LEN);
	/* �ƶ�һ��ID�ֽ�+�û����ֽڳ��ȵĵ�ַ����д����� */
	W25QXX_Write(&balance, index + ID_LEN + NAME_LEN, BALANCE_LEN);
	return TRUE;
}

/**
 * @brief ��ʾɾ���û��˵�
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
 * @brief ɾ���û�
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

		/* ��KEY1ɾ��ָ���û���ID ��ˢָ���Ŀ�����ɾ������ */
		while (keyFun == KEY1_VALUE)
		{
			LCD_Clear(BLUE);
			ReadCardTips();
			status = ReadCard(); // ˢ��
			if (status == MI_OK)
			{
				RemoveUser(cardID);
				keyFun = KEY_NULL;
			}
			else /* ����ʧ�� */
			{
				keyFun = KEY_NULL; // ǿ���˳�
			}
		}

		/* ɾ�������û���ID */
		if (keyFun == KEY2_VALUE)
		{
			RemoveAllUser();
			keyFun = KEY_NULL;
		}
	}
}

/**
 * @brief ɾ�������û�
 */
void RemoveAllUser()
{
	keyFun = KEY_NULL;
	LCD_Clear(BLUE);
	/* ��ȷ���Ƿ����ɾ�������û���ID�Ĳ��� */
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
 * @brief ɾ�������û�����
 * @param startAddr[u32] ���ݴ洢������ʼ��ַ
 */
void RemoveAllData(u32 startAddr)
{
	u32 i;
	u8 IDValue[DATA_SIZE];
	u8 nullData[DATA_SIZE];
	memset(nullData, 0xFF, DATA_SIZE); // ��IDֵ
	for (i = startAddr; i <= MAX_ADDR; i += DATA_SIZE)
	{
		W25QXX_Read(IDValue, i, DATA_SIZE);
		/* �������� */
		if (IsEquals(IDValue, nullData, DATA_SIZE) == TRUE)
		{
			return; // ��������IDʱ ��Ϊ�����û�����ɾ�����
		}
		W25QXX_Write(nullData, i, DATA_SIZE);
	}
}

/**
 * @brief ͨ��������û���IDɾ��ָ���û�����
 * @param cardID
 */
void RemoveUser(u8 *cardID)
{
	/* ɾ���ɹ� */
	if (RemoveData(cardID) == TRUE)
	{
		LCD_Clear(BLUE);
		LCD_ShowString(10, 110, 220, 16, 16, "OK:Delete user succeed!");
		OK_BEEP();
		delay_ms(1000);
		LCD_Clear(BLUE);
	}
	else /* �޴��û� ɾ��ʧ�� */
	{
		LCD_Clear(BLUE);
		LCD_ShowString(10, 110, 220, 16, 16, "Error:Not fuond user! ");
		ERR_BEEP();
		delay_ms(1000);
		LCD_Clear(BLUE);
	}
}

/**
 * @brief ɾ��ָ���û�����
 * @param cardID[u8*]
 * @return Boolean
 */
Boolean RemoveData(u8 *cardID)
{
	u32 i, index;
	u8 userData[DATA_SIZE]; // ���ڴ洢 ID �û��� �˻����
	Boolean check;
	check = SearchID(cardID, &index); // �������û���ID
	/* ���ݴ洢�����ڴ��û���ID -> ɾ�� */
	if (check == TRUE)
	{
		/* ��������û�������ǰ�ƶ����� ���ɽ����û����� */
		for (i = index; i <= MAX_ADDR; i += DATA_SIZE)
		{
			/* Flash[i] = Flash[i + DATA_SIZE] */
			W25QXX_Read(userData, i + DATA_SIZE, DATA_SIZE);
			W25QXX_Write(userData, i, DATA_SIZE);
			/* �������� */
			if (IsNullData(userData, DATA_SIZE) == TRUE)
			{
				return TRUE; // �ƶ����� ɾ���ɹ� ����TRUE
			}
		}
	}
	/* �û������� ɾ��ʧ�� �򷵻�FALSE */
	return FALSE;
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
 * @brief BusChargeSystem(BCS)��ʼ��
 */
void BCSInit(void)
{
	LCD_Clear(BLUE);   // ����Ϊ��ɫ ��������ɫ
	BACK_COLOR = BLUE; // ���ñ�����ɫΪ��ɫ
	POINT_COLOR = RED; // ��������Ϊ��ɫ
	keyFun = KEY_NULL; // �ȳ�ʼ��û���κΰ������µ�״̬
}
