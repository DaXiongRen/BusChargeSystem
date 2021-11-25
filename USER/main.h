#ifndef _MAIN_H
#define _MAIN_H
#include "sys.h"

/* �û���ID���� */
#define ID_LEN 4

/* �����ݴ洢�����û���ID���ڵı�־ */
#define EXIST 1

/* ѡ�����ڵ�ģʽ */
#define D 0
#define H 1
#define M 2
#define DHM 3

/* ���岼������ */
typedef enum
{
    TRUE,
    FALSE
} Boolean;

/* ������������� */
extern u8 keyFun; //keyFunȡֵ -> KEY_NULL:�޲��� KEY0_VALUE:���ж�����Ϣ KEY1_VALUE:����û��� KEY2_VALUE:ɾ���û���
extern u8 CardID[ID_LEN];

/* ���������к����ӿ� */
void DCSInit(void);                   // DoorControlSystem(DCS)��ʼ��
void DCSRunning(void);                // ϵͳ�������
void UserLogin(void);                 // �û���¼
void LcdDesktop(void);                // �û�����
u32 SerachID(u8 *CardID);             // �����û���ID
Boolean IsEquals(u8 *arr1, u8 *arr2); // ����Ƚ�
u8 ShowTime(u8 timeMode);             // ��ʾʱ��
void ShowMenu(void);                  // ��ʾ����û��˵�
void AddUserID(void);                 // ����û�ID
u8 DeleteUser(void);                  // ɾ���û�
Boolean RemoveID(u8 *ID);             // ɾ��ָ��ID
Boolean RemoveAllID(void);            // ɾ������ID
char ReadCard(void);                  // ����

#endif
